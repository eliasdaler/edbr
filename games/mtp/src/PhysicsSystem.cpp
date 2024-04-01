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
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <cstdarg>
#include <iostream>

#include <edbr/DevTools/Im3dState.h>
#include <edbr/Graphics/CPUMesh.h>
#include <edbr/Input/InputManager.h>
#include <edbr/Util/Im3dUtil.h>
#include <edbr/Util/JoltUtil.h>

#include <im3d.h>

#include <imgui.h>

#include <set>

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
            JPH::RVec3(0.0, -1.0, 0.0),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Static,
            Layers::NON_MOVING);
        floor = bodyInterface.CreateBody(floorSettings);
        bodyInterface.AddBody(floor->GetID(), JPH::EActivation::DontActivate);
    }

    createCharacter(characterParams);
}

void PhysicsSystem::drawDebugShapes(const Camera& camera)
{
    debugRenderer.setCamera(camera);
    physicsSystem.DrawBodies(debugRenderer.bodyDrawSettings, &debugRenderer);

    if (debugRenderer.bodyDrawSettings.mDrawShape) {
        // draw character
        const auto com = character->GetCenterOfMassTransform();
        const auto worldTransform = character->GetWorldTransform();
        character->GetShape()->Draw(
            &debugRenderer, com, JPH::Vec3::sReplicate(1.0f), JPH::Color::sGreen, false, true);
        // draw character's shape
        debugRenderer.DrawCapsule(
            com,
            0.5f * characterParams.characterHeight,
            characterParams.characterRadius + character->GetCharacterPadding(),
            JPH::Color::sGrey,
            JPH::DebugRenderer::ECastShadow::Off,
            JPH::DebugRenderer::EDrawMode::Wireframe);
    }
}

void PhysicsSystem::handleCharacterInput(
    float dt,
    glm::vec3 movementDirection,
    bool jumping,
    bool running)
{
    JPH::Vec3 movementDir = util::glmToJolt(movementDirection);

    bool playerControlsHorizontalVelocity =
        characterParams.controlMovementDuringJump || character->IsSupported();

    if (playerControlsHorizontalVelocity) {
        auto charSpeed =
            running ? characterParams.characterSpeedRun : characterParams.characterSpeedWalk;
        if (character->GetGroundState() == JPH::CharacterVirtual::EGroundState::InAir) {
            charSpeed = characterParams.characterSpeedRun * 0.75f;
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
    bool onGround = character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;
    bool useGroundVelocity = characterParams.enableCharacterInertia ?
                                 movingTowardsGround :
                                 !character->IsSlopeTooSteep(character->GetGroundNormal());
    if (onGround && useGroundVelocity) {
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
    newVelocity +=
        (characterUpRotation * physicsSystem.GetGravity() * characterParams.gravityFactor) * dt;

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
}

void PhysicsSystem::update(float dt, const glm::quat& characterRotation)
{
    characterContactListener.startFrame();

    characterPreUpdate(dt, characterRotation);

    static const auto defaultStep = 1 / 60.f;
    const auto collisionSteps = (int)std::ceil(defaultStep / dt);

    // Step the world
    physicsSystem.Update(dt, collisionSteps, tempAllocator.get(), &job_system);

    sendCollisionEvents();
}

void PhysicsSystem::sendCollisionEvents()
{
    for (const auto& bodyID : characterContactListener.contacts) {
        if (!characterContactListener.prevFrameContacts.contains(bodyID)) {
            // printf("character started collision with %d\n", (int)bodyID.GetIndex());
            auto e = getEntityByBodyID(bodyID);
            if (e.entity() != entt::null) {
                //  printf("entity: %d\n", (int)e.entity());
            }
        }
    }

    for (const auto& bodyID : characterContactListener.prevFrameContacts) {
        if (!characterContactListener.contacts.contains(bodyID)) {
            // printf("character ended collision with %d\n", (int)bodyID.GetIndex());
            auto e = getEntityByBodyID(bodyID);
            if (e.entity() != entt::null) {
                // printf("entity: %d\n", (int)e.entity());
            }
        }
    }
}

void PhysicsSystem::characterPreUpdate(float dt, const glm::quat& characterRotation)
{
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

    JPH::CollideShapeSettings settings;

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

    if (drawCollisionShapeWireframe) {
        debugRenderer.DrawSphere(sphereCenterPos, interactionSphereRadius, JPH::Color::sBlue);
    }

    {
        bool had_hit = !collector.mHits.empty();
        if (had_hit) {
            // Draw results
            for (const auto& hit : collector.mHits) {
                JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), hit.mBodyID2);
                if (lock.Succeeded()) {
                    const JPH::Body& hit_body = lock.GetBody();

                    if (drawCollisionShapeWireframe) {
                        // Draw bounding box
                        JPH::Color color =
                            hit_body.IsDynamic() ? JPH::Color::sRed : JPH::Color::sCyan;
                        debugRenderer.DrawWireBox(
                            hit_body.GetCenterOfMassTransform(),
                            hit_body.GetShape()->GetLocalBounds(),
                            color);
                    }
                }
            }
        }
    }
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

void PhysicsSystem::createCharacter(const CharacterParams& cp)
{
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

void PhysicsSystem::cleanup()
{
    JPH::BodyInterface& body_interface = physicsSystem.GetBodyInterface();

    body_interface.RemoveBody(floor->GetID());
    body_interface.DestroyBody(floor->GetID());

    tempAllocator.reset();

    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
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
        // meshSettings.Sanitize();

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
    bool trigger)
{
    auto& body_interface = physicsSystem.GetBodyInterface();
    auto settings = JPH::BodyCreationSettings(
        shape,
        util::glmToJolt(transform.getPosition()),
        util::glmToJolt(transform.getHeading()),
        staticBody ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic,
        staticBody ? Layers::NON_MOVING : Layers::MOVING);
    settings.mIsSensor = trigger;

    auto bodyID = body_interface.CreateAndAddBody(
        settings, staticBody ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
    auto [it, inserted] = bodyIDToEntity.emplace(bodyID.GetIndex(), e);
    assert(inserted);
    return bodyID;
}

void PhysicsSystem::updateTransform(JPH::BodyID id, const Transform& transform)
{
    auto& body_interface = physicsSystem.GetBodyInterface();
    auto mt = body_interface.GetMotionType(id);
    body_interface.SetPositionAndRotation(
        id,
        util::glmToJolt(transform.getPosition()),
        util::glmToJolt(transform.getHeading()),
        mt == JPH::EMotionType::Dynamic ? JPH::EActivation::Activate :
                                          JPH::EActivation::DontActivate);

    if (transform.getScale() != glm::vec3{1.f}) {
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
    ImGui::Checkbox("Draw collision lines with depth", &drawCollisionLinesWithDepth);
    ImGui::Checkbox("Draw collision wireframe", &drawCollisionShapeWireframe);
    ImGui::Checkbox("Draw collision BB", &drawCollisionShapeBoundingBox);

    if (im.getKeyboard().wasJustPressed(SDL_SCANCODE_B)) {
        drawCollisionShapeWireframe = !drawCollisionShapeWireframe;
    }

    debugRenderer.setDrawLinesDepthTest(drawCollisionLinesWithDepth);
    debugRenderer.setDrawShapeWireFrame(drawCollisionShapeWireframe);
    debugRenderer.setDrawBoundingBox(drawCollisionShapeBoundingBox);

    if (ImGui::CollapsingHeader("Character")) {
        bool creationParamChanged = false;

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
        ImGui::DragFloat("Run speed", &characterParams.characterSpeedRun, 0.1f);
        ImGui::DragFloat("Walk speed", &characterParams.characterSpeedWalk, 0.1f);
        ImGui::DragFloat("Jump speed", &characterParams.jumpSpeed, 0.1f);
        ImGui::Checkbox("Control during jump", &characterParams.controlMovementDuringJump);
        ImGui::DragFloat("Gravity factor", &characterParams.gravityFactor, 0.1f, 0.5f, 5.f);

        if (creationParamChanged) {
            const auto prevPos = character->GetPosition();
            createCharacter(characterParams);
            character->SetPosition(prevPos);
        }
    }
}

entt::handle PhysicsSystem::getEntityByBodyID(const JPH::BodyID& bodyID) const
{
    auto it = bodyIDToEntity.find(bodyID.GetIndex());
    if (it != bodyIDToEntity.end()) {
        return it->second;
    }
    return {};
}
