#include "PhysicsSystem.h"

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>

#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/RayCast.h>

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <tracy/Tracy.hpp>

#include <cstdarg>
#include <iostream>
#include <set>

#include <edbr/DevTools/Im3dState.h>
#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/ECS/Components/SceneComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>
#include <edbr/Event/EventManager.h>
#include <edbr/Graphics/CPUMesh.h>
#include <edbr/Input/InputManager.h>
#include <edbr/SceneCache.h>
#include <edbr/Util/Im3dUtil.h>
#include <edbr/Util/JoltUtil.h>

#include "Components.h"
#include "Events.h"

#include <im3d.h>
#include <imgui.h>

#include <fmt/format.h>

namespace
{

struct CharacterContactListener : public JPH::CharacterContactListener {
    bool mAllowSliding{false};
    JPH::PhysicsSystem* physicsSystem;

    std::set<JPH::BodyID> prevFrameContacts;
    std::set<JPH::BodyID> contacts;

    void startFrame()
    {
        prevFrameContacts = contacts;
        contacts.clear();
    }

    void OnContactSolve(
        const JPH::CharacterVirtual* inCharacter,
        const JPH::BodyID& inBodyID2,
        const JPH::SubShapeID& inSubShapeID2,
        JPH::RVec3Arg inContactPosition,
        JPH::Vec3Arg inContactNormal,
        JPH::Vec3Arg inContactVelocity,
        const JPH::PhysicsMaterial* inContactMaterial,
        JPH::Vec3Arg inCharacterVelocity,
        JPH::Vec3& ioNewCharacterVelocity) override
    {
        // Don't allow the player to slide down static not-too-steep surfaces
        // when not actively moving and when not on a moving platform
        if (!mAllowSliding && inContactVelocity.IsNearZero() &&
            !inCharacter->IsSlopeTooSteep(inContactNormal))
            ioNewCharacterVelocity = JPH::Vec3::sZero();
    }

    void OnContactAdded(
        const JPH::CharacterVirtual* inCharacter,
        const JPH::BodyID& inBodyID2,
        const JPH::SubShapeID& inSubShapeID2,
        JPH::RVec3Arg inContactPosition,
        JPH::Vec3Arg inContactNormal,
        JPH::CharacterContactSettings& ioSettings) override
    {
        contacts.insert(inBodyID2);

        // If we encounter an object that can push us, enable sliding
        if (ioSettings.mCanPushCharacter &&
            physicsSystem->GetBodyInterface().GetMotionType(inBodyID2) != JPH::EMotionType::Static)
            mAllowSliding = true;
    }
};

static CharacterContactListener characterContactListener;

}

static void TraceImpl(const char* inFMT, ...)
{
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);
    std::cout << buffer << std::endl;
}

#ifdef JPH_ENABLE_ASSERTS

static bool AssertFailedImpl(
    const char* inExpression,
    const char* inMessage,
    const char* inFile,
    std::uint32_t inLine)
{
    // Print to the TTY
    std::cout << inFile << ":" << inLine << ": (" << inExpression << ") "
              << (inMessage != nullptr ? inMessage : "") << std::endl;

    // Breakpoint
    return true;
};

#endif // JPH_ENABLE_ASSERTS

void PhysicsSystem::InitStaticObjects()
{
    JPH::RegisterDefaultAllocator();

    // Install trace and assert callbacks
    JPH::Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
}

PhysicsSystem::PhysicsSystem(EventManager& eventManager) : eventManager(eventManager)
{}

void PhysicsSystem::init()
{
    tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

    job_system.Init(
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

    const std::uint32_t cMaxBodies = 65536;
    const std::uint32_t cNumBodyMutexes = 0;
    const std::uint32_t cMaxBodyPairs = 65536;
    const std::uint32_t cMaxContactConstraints = 10240;

    physicsSystem.Init(
        cMaxBodies,
        cNumBodyMutexes,
        cMaxBodyPairs,
        cMaxContactConstraints,
        bpLayerInterface,
        objectVsBPLayerFilter,
        objectVsObjectPairFilter);

    physicsSystem.SetBodyActivationListener(&bodyActivationListener);
    physicsSystem.SetContactListener(&contactListener);

    characterContactListener.physicsSystem = &physicsSystem;

    { // create floor
        JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();
        JPH::BoxShapeSettings floorShapeSettings(JPH::Vec3(100.0f, 1.0f, 100.0f));
        JPH::ShapeRefC floorShape = floorShapeSettings.Create().Get();
        JPH::BodyCreationSettings floorSettings(
            floorShape,
            JPH::RVec3(0.0, -2.0, 0.0),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Static,
            Layers::NON_MOVING);
        floor = bodyInterface.CreateBody(floorSettings);
        bodyInterface.AddBody(floor->GetID(), JPH::EActivation::DontActivate);
    }

    eventManager.addListener(this, &PhysicsSystem::onEntityTeleported);
}

void PhysicsSystem::drawDebugShapes(const Camera& camera)
{
    debugRenderer.setCamera(camera);
    drawBodies(camera);
}

void PhysicsSystem::drawBodies(const Camera& camera)
{
    if (!debugRenderer.bodyDrawSettings.mDrawShape) {
        return;
    }

    static JPH::BodyIDVector body_ids; // store as static to not realloc each frame
    body_ids.clear();
    physicsSystem.GetBodies(body_ids);

    auto& lockInterface = physicsSystem.GetBodyLockInterfaceNoLock();
    for (const auto id : body_ids) {
        JPH::BodyLockRead lock(lockInterface, id);
        if (!lock.Succeeded()) {
            continue;
        }

        const auto& body = lock.GetBody();
        if (!body.IsSensor() && drawSensorsOnly) {
            continue;
        }

        if (body.IsSensor()) {
            body.GetShape()->Draw(
                &debugRenderer,
                body.GetCenterOfMassTransform(),
                JPH::Vec3::sReplicate(1.0f),
                JPH::Color::sGetDistinctColor(body.GetID().GetIndex()),
                false,
                false);
            // shape of triggers is more understandable with wireframes
            body.GetShape()->Draw(
                &debugRenderer,
                body.GetCenterOfMassTransform(),
                JPH::Vec3::sReplicate(1.0f),
                JPH::Color::sGetDistinctColor(body.GetID().GetIndex()),
                false,
                true);
        } else {
            body.GetShape()->Draw(
                &debugRenderer,
                body.GetCenterOfMassTransform(),
                JPH::Vec3::sReplicate(1.0f),
                JPH::Color::sGetDistinctColor(body.GetID().GetIndex()),
                false,
                debugRenderer.bodyDrawSettings.mDrawShapeWireframe);
        }
    }

    // draw character
    if (drawCharacterShape) {
        const auto com = character->GetCenterOfMassTransform();
        const auto worldTransform = character->GetWorldTransform();
        character->GetShape()->Draw(
            &debugRenderer, com, JPH::Vec3::sReplicate(1.0f), JPH::Color::sGreen, false, true);
    }
}

void PhysicsSystem::handleCharacterInput(
    float dt,
    glm::vec3 movementDirection,
    bool jumping,
    bool jumpHeld,
    bool running)
{
    JPH::Vec3 movementDir = util::glmToJolt(movementDirection);

    bool playerControlsHorizontalVelocity =
        characterParams.controlMovementDuringJump || character->IsSupported();

    if (playerControlsHorizontalVelocity) {
        auto charSpeed = running ? characterParams.runSpeed : characterParams.walkSpeed;
        if (character->GetGroundState() == JPH::CharacterVirtual::EGroundState::InAir) {
            charSpeed = characterParams.runSpeed * 0.8f;
        }
        // Smooth the player input (if uses inertia)
        characterDesiredVelocity =
            characterParams.enableCharacterInertia ?
                0.25f * movementDir * charSpeed + 0.75f * characterDesiredVelocity :
                movementDir * charSpeed;

        // True if the player intended to move
        characterContactListener.mAllowSliding = !movementDir.IsNearZero();
    } else {
        // While in air we allow sliding
        characterContactListener.mAllowSliding = true;
    }

    // A cheaper way to update the character's ground velocity,
    // the platforms that the character is standing on may have changed velocity
    character->UpdateGroundVelocity();

    // Determine new basic velocity
    const auto currentVerticalVelocity =
        character->GetLinearVelocity().Dot(character->GetUp()) * character->GetUp();
    const auto groundVelocity = character->GetGroundVelocity();
    JPH::Vec3 newVelocity;
    bool movingTowardsGround = (currentVerticalVelocity.GetY() - groundVelocity.GetY()) < 0.1f;
    characterOnGround =
        character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;
    bool useGroundVelocity = characterParams.enableCharacterInertia ?
                                 movingTowardsGround :
                                 !character->IsSlopeTooSteep(character->GetGroundNormal());
    if (characterOnGround && useGroundVelocity) {
        // Assume velocity of ground when on ground
        newVelocity = groundVelocity;
        // Jump
        if (jumping && movingTowardsGround) {
            newVelocity += characterParams.jumpSpeed * character->GetUp();
        }
    } else {
        newVelocity = currentVerticalVelocity;
    }

    // Gravity
    const auto characterUpRotation = JPH::Quat::sIdentity();
    auto gravity = physicsSystem.GetGravity() * characterParams.gravityFactor;
    if (!jumpHeld && newVelocity.GetY() > 0.f) {
        gravity *= characterParams.smallJumpFactor;
    }
    newVelocity += (characterUpRotation * gravity) * dt;

    if (playerControlsHorizontalVelocity) {
        // Player input
        newVelocity += characterUpRotation * characterDesiredVelocity;
    } else {
        // Preserve horizontal velocity
        JPH::Vec3 currentHorizontalVelocity =
            character->GetLinearVelocity() - currentVerticalVelocity;
        newVelocity += currentHorizontalVelocity;
    }

    // Update character velocity
    character->SetLinearVelocity(newVelocity);

    handledPlayerInputThisFrame = true;
}

void PhysicsSystem::update(float dt, const glm::quat& characterRotation)
{
    if (character) {
        if (!handledPlayerInputThisFrame) {
            handleCharacterInput(dt, {}, false, false, false);
        }
        handledPlayerInputThisFrame = false;
        characterContactListener.startFrame();
        characterPreUpdate(dt, characterRotation);
    }

    static const auto defaultStep = 1 / 60.f;
    const auto collisionSteps = (int)std::ceil(defaultStep / dt);

    // Step the world
    {
        ZoneScopedN("Jolt physics system update");
        physicsSystem.Update(dt, collisionSteps, tempAllocator.get(), &job_system);
    }

    if (character) {
        collectInteractableEntities(characterRotation);
    }

    sendCollisionEvents();
}

void PhysicsSystem::collectInteractableEntities(const glm::quat& characterRotation)
{
    /*
    JPH::AllHitCollisionCollector<JPH::CollideShapeBodyCollector> collector;
    physicsSystem.GetBroadPhaseQuery().CollideSphere(util::glmToJolt(pos), 0.5f, collector);
    */

    /*
    auto startPos = util::glmToJolt(getCharacterPosition() + glm::vec3{0.f, 1.f, 0.f});
    auto direction = util::glmToJolt(characterRotation * glm::vec3{0.f, 0.f, 1.f} * 1.5f);
    JPH::RRayCast ray{startPos, direction};
    debugRenderer.DrawLine(startPos, startPos + direction, JPH::Color::sGreen);
    */

    /*
    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
    JPH::RayCastSettings settings;
    settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;
    physicsSystem.GetNarrowPhaseQuery().CastRay(ray, settings, collector);
    */

    interactableEntities.clear();

    JPH::CollideShapeSettings settings;
    settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;

    // sync interaction sphere
    const auto sphereCenterPos = util::glmToJolt(
        getCharacterPosition() + math::GlobalUpAxis * interactionSphereOffset.y +
        characterRotation * math::GlobalFrontAxis * interactionSphereOffset.z);

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
    physicsSystem.GetNarrowPhaseQuery().CollideShape(
        characterInteractionShape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sTranslation(sphereCenterPos),
        settings,
        JPH::RVec3(),
        collector);

    if (drawCharacterShape) {
        debugRenderer.DrawWireSphere(sphereCenterPos, interactionSphereRadius, JPH::Color::sBlue);
    }

    for (const auto& hit : collector.mHits) {
        JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), hit.mBodyID2);
        if (!lock.Succeeded()) {
            continue;
        }

        const JPH::Body& hit_body = lock.GetBody();
        auto it = bodyIDToEntity.find(hit_body.GetID().GetIndex());
        if (it != bodyIDToEntity.end()) {
            const auto& e = it->second;
            if (e.all_of<InteractComponent>()) {
                interactableEntities.push_back(e);

                if (drawCharacterShape) {
                    // Draw bounding box of hit entity
                    JPH::Color color = hit_body.IsDynamic() ? JPH::Color::sRed : JPH::Color::sCyan;
                    debugRenderer.DrawWireBox(
                        hit_body.GetCenterOfMassTransform(),
                        hit_body.GetShape()->GetLocalBounds(),
                        color);
                }
            }
        }
    }
}

void PhysicsSystem::sendCollisionEvents()
{
    for (const auto& bodyID : characterContactListener.contacts) {
        if (!characterContactListener.prevFrameContacts.contains(bodyID)) {
            // printf("character started collision with %d\n", (int)bodyID.GetIndex());
            auto e = getEntityByBodyID(bodyID);
            if (e.entity() != entt::null) {
                //  printf("entity: %d\n", (int)e.entity());
                CharacterCollisionStartedEvent event;
                event.entity = e;
                eventManager.triggerEvent(event);
            }
        }
    }

    for (const auto& bodyID : characterContactListener.prevFrameContacts) {
        if (!characterContactListener.contacts.contains(bodyID)) {
            // printf("character ended collision with %d\n", (int)bodyID.GetIndex());
            auto e = getEntityByBodyID(bodyID);
            if (e.entity() != entt::null) {
                // printf("entity: %d\n", (int)e.entity());
                CharacterCollisionEndedEvent event;
                event.entity = e;
                eventManager.triggerEvent(event);
            }
        }
    }
}

void PhysicsSystem::characterPreUpdate(float dt, const glm::quat& characterRotation)
{
    ZoneScopedN("Jolt character update");

    JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;

    if (!characterParams.enableStickToFloor) {
        updateSettings.mStickToFloorStepDown = JPH::Vec3::sZero();
    } else {
        updateSettings.mStickToFloorStepDown =
            -character->GetUp() * updateSettings.mStickToFloorStepDown.Length();
    }

    if (!characterParams.enableWalkStairs) {
        updateSettings.mWalkStairsStepUp = JPH::Vec3::sZero();
    } else {
        updateSettings.mWalkStairsStepUp =
            character->GetUp() * updateSettings.mWalkStairsStepUp.Length();
    }

    // Update the character position
    const auto gravity =
        -character->GetUp() * physicsSystem.GetGravity().Length() * characterParams.gravityFactor;
    character->ExtendedUpdate(
        dt,
        gravity,
        updateSettings,
        physicsSystem.GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
        physicsSystem.GetDefaultLayerFilter(Layers::MOVING),
        {},
        {},
        *tempAllocator);
}

namespace
{
JPH::Ref<JPH::Shape> createCharacterShape(float capsuleHeight, float capsuleRadius)
{
    return JPH::RotatedTranslatedShapeSettings(
               JPH::Vec3(0, 0.5f * capsuleHeight + capsuleRadius, 0),
               JPH::Quat::sIdentity(),
               new JPH::CapsuleShape(0.5f * capsuleHeight, capsuleRadius))
        .Create()
        .Get();
}
}

void PhysicsSystem::createCharacter(entt::handle e, const VirtualCharacterParams& cp)
{
    characterEntity = e;

    JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
    settings->mMaxSlopeAngle = cp.maxSlopeAngle;
    settings->mMaxStrength = cp.maxStrength;
    settings->mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;
    settings->mCharacterPadding = cp.characterPadding;
    settings->mPenetrationRecoverySpeed = cp.penetrationRecoverySpeed;
    settings->mPredictiveContactDistance = cp.predictiveContactDistance;
    // Accept contacts that touch the lower sphere of the capsule
    settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -cp.characterRadius);

    character = new JPH::
        CharacterVirtual(settings, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), 0, &physicsSystem);

    characterShape = createCharacterShape(cp.characterHeight, cp.characterRadius);
    character->SetShape(characterShape, FLT_MAX, {}, {}, {}, {}, *tempAllocator);

    // interaction sphere
    characterInteractionShape = new JPH::SphereShape(interactionSphereRadius);

    character->SetListener(&characterContactListener);
}

void PhysicsSystem::stopCharacterMovement()
{
    character->SetLinearVelocity({});
}

void PhysicsSystem::setCharacterPosition(const glm::vec3 pos)
{
    character->SetPosition(util::glmToJolt(pos));
}

glm::vec3 PhysicsSystem::getCharacterPosition() const
{
    return util::joltToGLM(character->GetPosition());
}

glm::vec3 PhysicsSystem::getCharacterVelocity() const
{
    return util::joltToGLM(character->GetLinearVelocity());
}

bool PhysicsSystem::isCharacterOnGround() const
{
    // NOTE: we have to compute this before physics update and not poll here
    // otherwise this might be wrong if just calling character->GetGroundState!
    return characterOnGround;
}

void PhysicsSystem::cleanup()
{
    eventManager.removeListener<EntityTeleportedEvent>(this);

    JPH::BodyInterface& body_interface = physicsSystem.GetBodyInterface();

    body_interface.RemoveBody(floor->GetID());
    body_interface.DestroyBody(floor->GetID());

    tempAllocator.reset();

    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

void PhysicsSystem::addEntity(entt::handle e, SceneCache& sceneCache)
{
    auto& pc = e.get<PhysicsComponent>();
    assert(pc.bodyId.IsInvalid() && "entity was already to physics system before");

    JPH::Ref<JPH::Shape> shape;

    switch (pc.bodyType) {
    case PhysicsComponent::BodyType::Sphere: {
        const auto& params = std::get<PhysicsComponent::SphereParams>(pc.bodyParams);
        shape = new JPH::SphereShape(params.radius);
    } break;
    case PhysicsComponent::BodyType::Capsule: {
        const auto& params = std::get<PhysicsComponent::CapsuleParams>(pc.bodyParams);
        shape = new JPH::CapsuleShape(params.halfHeight, params.radius);
        if (pc.originType == PhysicsComponent::OriginType::BottomPlane) {
            shape = new JPH::RotatedTranslatedShape(
                {0.f, params.halfHeight + params.radius, 0.f}, JPH::Quat().sIdentity(), shape);
        }
    } break;
    case PhysicsComponent::BodyType::Cylinder: {
        const auto& params = std::get<PhysicsComponent::CylinderParams>(pc.bodyParams);
        shape = new JPH::CylinderShape(params.halfHeight, params.radius);
        if (pc.originType == PhysicsComponent::OriginType::BottomPlane) {
            shape = new JPH::RotatedTranslatedShape(
                {0.f, params.halfHeight, 0.f}, JPH::Quat().sIdentity(), shape);
        }
    } break;
    case PhysicsComponent::BodyType::AABB: {
        auto& params = std::get<PhysicsComponent::AABBParams>(pc.bodyParams);

        // need to compute AABB from the mesh if not initialized still
        if (params.min == params.max && params.min == glm::vec3{}) {
            const auto& mc = e.get<MeshComponent>();
            const auto& sc = e.get<SceneComponent>();
            const auto& scene = sceneCache.loadOrGetScene(sc.creationSceneName);
            const auto aabb = edbr::calculateBoundingBoxLocal(scene, mc.meshes);
            params.min = aabb.min;
            params.max = aabb.max;
        }

        auto size = glm::abs(params.max - params.min);
        // almost flat - needs some height to be valid
        bool almostZeroHeight = 0.f;
        if (size.y < 0.001f) {
            size.y = 0.1f;
            almostZeroHeight = true;
        }

        // for completely flat objects, the origin will be at the center of the object
        // for objects with height, it will be at the center of the bottom AABB plane
        shape = new JPH::BoxShape(util::glmToJolt(size / 2.f), 0.f);
        auto offsetY = almostZeroHeight ? -size.y / 2.f : size.y / 2.f;
        if (pc.originType == PhysicsComponent::OriginType::Center) {
            offsetY = 0.f;
        }
        shape =
            new JPH::RotatedTranslatedShape({0.f, offsetY, 0.f}, JPH::Quat().sIdentity(), shape);
    } break;
    case PhysicsComponent::BodyType::ConvexHull: {
        const auto& sc = e.get<SceneComponent>();
        const auto& scene = sceneCache.loadOrGetScene(sc.creationSceneName);
        const auto& mc = e.get<MeshComponent>();
        assert(mc.meshes.size() == 1 && "TODO: support multiple meshes");
        const auto& mesh = scene.cpuMeshes.at(mc.meshes[0]);
        JPH::Array<JPH::Vec3> vertices(mesh.vertices.size());
        for (const auto& v : mesh.vertices) {
            vertices.push_back(util::glmToJolt(v.position));
        }
        auto res = JPH::ConvexHullShapeSettings(vertices).Create();
        if (res.HasError()) {
            fmt::println("failed to make convex hull shape: {}", res.GetError());
            return;
        }
        shape = res.Get();
    } break;
    case PhysicsComponent::BodyType::TriangleMesh: {
        const auto& sc = e.get<SceneComponent>();
        const auto& scene = sceneCache.loadOrGetScene(sc.creationSceneName);
        std::vector<const CPUMesh*> meshes;
        const auto& mc = e.get<MeshComponent>();
        for (const auto& meshId : mc.meshes) {
            meshes.push_back(&scene.cpuMeshes.at(meshId));
        }
        shape = cacheMeshShape(meshes, mc.meshes, mc.meshTransforms);
    } break;
    case PhysicsComponent::BodyType::VirtualCharacter: {
        characterParams = std::get<VirtualCharacterParams>(pc.bodyParams);
        createCharacter(e, characterParams);
        return;
    } break;
    default:
        break;
    }

    assert(shape);

    bool staticBody = pc.type == PhysicsComponent::Type::Static;
    assert(pc.type != PhysicsComponent::Type::Kinematic && "TODO");
    pc.bodyId = createBody(e, e.get<TransformComponent>().transform, shape, staticBody, pc.sensor);
    assert(!pc.bodyId.IsInvalid());
}

JPH::Ref<JPH::Shape> PhysicsSystem::cacheMeshShape(
    const std::vector<const CPUMesh*>& meshes,
    const std::vector<MeshId>& meshIds,
    const std::vector<Transform>& meshTransforms)
{
    for (const auto& cachedMesh : cachedMeshShapes) {
        if (cachedMesh.meshIds == meshIds && cachedMesh.meshTransforms == meshTransforms) {
            return cachedMesh.meshShape;
        }
    }

    JPH::StaticCompoundShapeSettings compoundShapeSettings;
    compoundShapeSettings.mSubShapes.reserve(meshes.size());
    for (std::size_t i = 0; i < meshes.size(); ++i) {
        const auto& mesh = *meshes[i];
        const auto& transform = meshTransforms[i];

        JPH::MeshShapeSettings meshSettings;
        meshSettings.mTriangleVertices.reserve(mesh.vertices.size());
        meshSettings.mIndexedTriangles.reserve(mesh.indices.size());
        for (std::size_t i = 0; i < mesh.indices.size(); i += 3) {
            auto triangle =
                JPH::IndexedTriangle{mesh.indices[i + 0], mesh.indices[i + 1], mesh.indices[i + 2]};
            meshSettings.mIndexedTriangles.push_back(std::move(triangle));
        }
        for (const auto& v : mesh.vertices) {
            auto pos = util::glmToJoltFloat3(v.position);
            meshSettings.mTriangleVertices.push_back(std::move(pos));
        }
        meshSettings.Sanitize();

        auto shapeRes = meshSettings.Create();
        if (!shapeRes.IsValid()) {
            printf("Error: %s\n", shapeRes.GetError().c_str());
            continue;
            // assert(false && shapeRes.GetError().c_str());
        }
        auto scaledShape = shapeRes.Get()->ScaleShape(util::glmToJolt(transform.getScale())).Get();
        compoundShapeSettings.AddShape(
            util::glmToJolt(transform.getPosition()),
            util::glmToJolt(transform.getHeading()),
            scaledShape);
    }
    assert(!compoundShapeSettings.mSubShapes.empty());

    auto compoundShape = compoundShapeSettings.Create();
    if (compoundShape.IsValid()) {
        cachedMeshShapes.push_back(CachedMeshShape{
            .meshIds = meshIds,
            .meshTransforms = meshTransforms,
            .meshShape = compoundShape.Get(),
        });
        return cachedMeshShapes.back().meshShape;
    }
    return nullptr;
}

JPH::BodyID PhysicsSystem::createBody(
    entt::handle e,
    const Transform& transform,
    JPH::Ref<JPH::Shape> shape,
    bool staticBody,
    bool sensor)
{
    if (transform.getScale() != glm::vec3{1.f}) {
        shape = shape->ScaleShape(util::glmToJolt(transform.getScale())).Get();
    }

    auto settings = JPH::BodyCreationSettings(
        shape,
        util::glmToJolt(transform.getPosition()),
        util::glmToJolt(transform.getHeading()),
        staticBody ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic,
        staticBody ? Layers::NON_MOVING : Layers::MOVING);
    settings.mIsSensor = sensor;

    auto& body_interface = physicsSystem.GetBodyInterface();
    auto bodyID = body_interface.CreateAndAddBody(
        settings, staticBody ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
    auto [it, inserted] = bodyIDToEntity.emplace(bodyID.GetIndex(), e);
    assert(inserted);

    return bodyID;
}

void PhysicsSystem::updateTransform(JPH::BodyID id, const Transform& transform, bool updateScale)
{
    auto& body_interface = physicsSystem.GetBodyInterface();
    auto mt = body_interface.GetMotionType(id);
    body_interface.SetPositionAndRotation(
        id,
        util::glmToJolt(transform.getPosition()),
        util::glmToJolt(transform.getHeading()),
        mt == JPH::EMotionType::Dynamic ? JPH::EActivation::Activate :
                                          JPH::EActivation::DontActivate);

    if (updateScale && transform.getScale() != glm::vec3{1.f}) {
        auto shape = body_interface.GetShape(id);
        body_interface.SetShape(
            id,
            shape->ScaleShape(util::glmToJolt(transform.getScale())).Get(),
            true,
            mt == JPH::EMotionType::Dynamic ? JPH::EActivation::Activate :
                                              JPH::EActivation::DontActivate);
    }
}

void PhysicsSystem::setVelocity(JPH::BodyID id, const glm::vec3& velocity)
{
    auto& body_interface = physicsSystem.GetBodyInterface();
    body_interface.SetLinearVelocity(id, util::glmToJolt(velocity));
}

void PhysicsSystem::syncCharacterTransform()
{
    if (character) {
        assert(characterEntity.valid());
        auto& tc = characterEntity.get<TransformComponent>();
        tc.transform.setPosition(getCharacterPosition());
    }
}

void PhysicsSystem::syncVisibleTransform(JPH::BodyID id, Transform& transform)
{
    auto& body_interface = physicsSystem.GetBodyInterface();
    auto mt = body_interface.GetMotionType(id);
    if (mt == JPH::EMotionType::Static) {
        return;
    }

    JPH::Vec3 position;
    JPH::Quat rotation;
    body_interface.GetPositionAndRotation(id, position, rotation);

    transform.setPosition(util::joltToGLM(position));
    transform.setHeading(util::joltToGLM(rotation));
}

void PhysicsSystem::doForBody(JPH::BodyID id, std::function<void(const JPH::Body&)> f)
{
    const auto& lock_interface = physicsSystem.GetBodyLockInterfaceNoLock();

    // Scoped lock
    {
        JPH::BodyLockRead lock(lock_interface, id);
        if (lock.Succeeded()) // body_id may no longer be valid
        {
            const JPH::Body& body = lock.GetBody();
            f(body);
        }
    }
}

void PhysicsSystem::updateDevUI(const InputManager& im, float dt)
{
    if (ImGui::Begin("Physics")) {
        ImGui::Checkbox("Draw collision lines with depth", &drawCollisionLinesWithDepth);
        ImGui::Checkbox("Draw collision shapes", &drawCollisionShapes);
        ImGui::Checkbox("Draw collision wireframe", &drawCollisionShapesWireframe);
        ImGui::Checkbox("Draw collision BB", &drawCollisionShapeBoundingBox);
        ImGui::Checkbox("Draw sensors only", &drawSensorsOnly);
        ImGui::Checkbox("Draw character shape", &drawCharacterShape);
        ImGui::DragFloat("Solid shape alpha", &debugRenderer.solidShapeAlpha, 0.01f, 0.f, 1.f);

        if (ImGui::CollapsingHeader("Character")) {
            bool creationParamChanged = false;

            ImGui::Text("Entity: %d", (int)characterEntity.entity());
            float slopeAngleDeg = glm::degrees(characterParams.maxSlopeAngle);
            if (ImGui::DragFloat("Max slope angle", &slopeAngleDeg, 0.01f, 0.f, 90.f)) {
                characterParams.maxSlopeAngle = glm::radians(slopeAngleDeg);
                creationParamChanged = true;
            }
            if (ImGui::DragFloat("Max strength", &characterParams.maxStrength, 1.f, 0.f, 1000.f)) {
                creationParamChanged = true;
            }
            if (ImGui::DragFloat(
                    "Penetration recovery",
                    &characterParams.penetrationRecoverySpeed,
                    0.1f,
                    0.f,
                    2.f)) {
                creationParamChanged = true;
            }
            if (ImGui::DragFloat(
                    "Pred. contact dist.",
                    &characterParams.predictiveContactDistance,
                    0.01f,
                    0.01f,
                    0.5f)) {
                creationParamChanged = true;
            }

            if (ImGui::DragFloat("Radius", &characterParams.characterRadius, 0.1f, 0.1f, 0.6f)) {
                creationParamChanged = true;
            }
            if (ImGui::DragFloat("Height", &characterParams.characterHeight, 0.1f, 0.5f, 2.f)) {
                creationParamChanged = true;
            }

            ImGui::Checkbox("Walk stairs", &characterParams.enableWalkStairs);
            ImGui::Checkbox("Stick to ground", &characterParams.enableStickToFloor);
            ImGui::Checkbox("Inertia", &characterParams.enableCharacterInertia);
            ImGui::DragFloat("Run speed", &characterParams.runSpeed, 0.1f);
            ImGui::DragFloat("Walk speed", &characterParams.walkSpeed, 0.1f);
            ImGui::DragFloat("Jump speed", &characterParams.jumpSpeed, 0.1f);
            ImGui::Checkbox("Control during jump", &characterParams.controlMovementDuringJump);
            ImGui::DragFloat("Gravity factor", &characterParams.gravityFactor, 0.1f, 0.5f, 5.f);
            ImGui::
                DragFloat("Small jump factor", &characterParams.smallJumpFactor, 0.1f, 0.5f, 20.f);

            if (creationParamChanged) {
                const auto prevPos = character->GetPosition();
                createCharacter(characterEntity, characterParams);
                character->SetPosition(prevPos);
            }
        }
    }
    ImGui::End();

    debugRenderer.drawLinesDepthTest = drawCollisionLinesWithDepth;
    debugRenderer.bodyDrawSettings.mDrawShape = drawCollisionShapes;
    debugRenderer.bodyDrawSettings.mDrawShapeWireframe = drawCollisionShapesWireframe;
    debugRenderer.bodyDrawSettings.mDrawBoundingBox = drawCollisionShapeBoundingBox;
}

entt::handle PhysicsSystem::getEntityByBodyID(const JPH::BodyID& bodyID) const
{
    auto it = bodyIDToEntity.find(bodyID.GetIndex());
    if (it != bodyIDToEntity.end()) {
        return it->second;
    }
    return {};
}

void PhysicsSystem::onEntityTeleported(const EntityTeleportedEvent& event)
{
    const auto entity = event.entity;
    if (character && entity == characterEntity) {
        setCharacterPosition(entity.get<TransformComponent>().transform.getPosition());
    } else {
        auto& pc = entity.get<PhysicsComponent>();
        updateTransform(pc.bodyId, entity.get<TransformComponent>().transform, false);
    }
}

void PhysicsSystem::onEntityDestroyed(entt::handle e)
{
    auto& pc = e.get<PhysicsComponent>();

    if (pc.bodyType == PhysicsComponent::BodyType::VirtualCharacter) {
        character = nullptr;
        characterEntity = {};
        return;
    }

    assert(!pc.bodyId.IsInvalid());

    // remove from bodyIDToEntity
    const auto it = bodyIDToEntity.find(pc.bodyId.GetIndex());
    assert(it != bodyIDToEntity.end());
    bodyIDToEntity.erase(it);

    // remove from simulation
    auto& bodyInterface = physicsSystem.GetBodyInterfaceNoLock();

    bodyInterface.RemoveBody(pc.bodyId);
    bodyInterface.DestroyBody(pc.bodyId);

    // reset body id
    pc.bodyId = JPH::BodyID{};
}
