#include "PhysicsSystem.h"

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <cstdarg>
#include <iostream>

#include <edbr/DevTools/Im3dState.h>
#include <edbr/Util/Im3dUtil.h>
#include <im3d.h>

static void TraceImpl(const char* inFMT, ...)
{
    // Format the message
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);

    // Print to the TTY
    std::cout << buffer << std::endl;
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(
    const char* inExpression,
    const char* inMessage,
    const char* inFile,
    uint inLine)
{
    // Print to the TTY
    std::cout << inFile << ":" << inLine << ": (" << inExpression << ") "
              << (inMessage != nullptr ? inMessage : "") << std::endl;

    // Breakpoint
    return true;
};

#endif // JPH_ENABLE_ASSERTS

void PhysicsSystem::init()
{
    // Register allocation hook. In this example we'll just let Jolt use malloc / free but you can
    // override these if you want (see Memory.h). This needs to be done before any other Jolt
    // function is called.
    JPH::RegisterDefaultAllocator();

    // Install trace and assert callbacks
    JPH::Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

    // Create a factory, this class is responsible for creating instances of classes based on their
    // name or hash and is mainly used for deserialization of saved data. It is not directly used in
    // this example but still required.
    JPH::Factory::sInstance = new JPH::Factory();

    // Register all physics types with the factory and install their collision handlers with the
    // CollisionDispatch class. If you have your own custom shape types you probably need to
    // register their handlers with the CollisionDispatch before calling this function. If you
    // implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it
    // before this function or else this function will create one for you.
    JPH::RegisterTypes();

    // We need a temp allocator for temporary allocations during the physics update. We're
    // pre-allocating 10 MB to avoid having to do allocations during the physics update.
    // B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
    // If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
    // malloc / free.
    temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

    // We need a job system that will execute physics jobs on multiple threads. Typically
    // you would implement the JobSystem interface yourself and let Jolt Physics run on top
    // of your own job scheduler. JobSystemThreadPool is an example implementation.
    job_system.Init(
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

    // This is the max amount of rigid bodies that you can add to the physics system. If you try to
    // add more you'll get an error. Note: This value is low because this is a simple test. For a
    // real project use something in the order of 65536.
    const uint cMaxBodies = 1024;

    // This determines how many mutexes to allocate to protect rigid bodies from concurrent access.
    // Set it to 0 for the default settings.
    const uint cNumBodyMutexes = 0;

    // This is the max amount of body pairs that can be queued at any time (the
    // broad phase will detect overlapping body pairs based on their bounding
    // boxes and will insert them into a queue for the narrowphase). If you
    // make this buffer too small the queue will fill up and the broad phase
    // jobs will start to do narrow phase work. This is slightly less
    // efficient. Note: This value is low because this is a simple test. For a
    // real project use something in the order of 65536.
    const uint cMaxBodyPairs = 1024;

    // This is the maximum size of the contact constraint buffer. If more
    // contacts (collisions between bodies) are detected than this number then
    // these contacts will be ignored and bodies will start interpenetrating /
    // fall through the world. Note: This value is low because this is a simple
    // test. For a real project use something in the order of 10240.
    const uint cMaxContactConstraints = 1024;

    // Now we can create the actual physics system.
    physics_system.Init(
        cMaxBodies,
        cNumBodyMutexes,
        cMaxBodyPairs,
        cMaxContactConstraints,
        broad_phase_layer_interface,
        object_vs_broadphase_layer_filter,
        object_vs_object_layer_filter);

    // A body activation listener gets notified when bodies activate and go to
    // sleep Note that this is called from a job so whatever you do here needs
    // to be thread safe.
    // Registering one is entirely optional.
    physics_system.SetBodyActivationListener(&body_activation_listener);

    // A contact listener gets notified when bodies (are about to) collide, and
    // when they separate again. Note that this is called from a job so
    // whatever you do here needs to be thread safe.
    // Registering one is entirely optional.
    physics_system.SetContactListener(&contact_listener);

    // The main way to interact with the bodies in the physics system is
    // through the body interface.  There is a locking and a non-locking
    // variant of this. We're going to use the locking version (even though
    // we're not planning to access bodies from multiple threads)
    JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();

    // Next we can create a rigid body to serve as the floor, we make a large
    // box Create the settings for the collision volume (the shape).
    // Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
    JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(100.0f, 1.0f, 100.0f));

    // Create the shape
    JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
    JPH::ShapeRefC floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you
                                                           // can check floor_shape_result for
                                                           // HasError() / GetError()

    // Create the settings for the body itself. Note that here you can also set
    // other properties like the restitution / friction.
    JPH::BodyCreationSettings floor_settings(
        floor_shape,
        JPH::RVec3(0.0, -1.0, 0.0),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Static,
        Layers::NON_MOVING);

    // Create the actual rigid body
    floor = body_interface.CreateBody(floor_settings); // Note that if we run out of
                                                       // bodies this can return nullptr

    // Add it to the world
    body_interface.AddBody(floor->GetID(), JPH::EActivation::DontActivate);

    // Now create a dynamic body to bounce on the floor
    // Note that this uses the shorthand version of creating and adding a body to the world
    JPH::BodyCreationSettings sphere_settings(
        new JPH::SphereShape(0.5f),
        JPH::RVec3(-54.0, 3.0, 0.0),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Layers::MOVING);
    sphere_id = body_interface.CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);

    JPH::BodyCreationSettings sphere2_settings(
        new JPH::SphereShape(0.5f),
        JPH::RVec3(-54.5, 3.8, 0.0),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Layers::MOVING);
    sphere2_id = body_interface.CreateAndAddBody(sphere2_settings, JPH::EActivation::Activate);

    JPH::BodyCreationSettings sphere3_settings(
        new JPH::SphereShape(0.5f),
        JPH::RVec3(-53.5, 4.8, 0.1),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Layers::MOVING);
    sphere3_id = body_interface.CreateAndAddBody(sphere3_settings, JPH::EActivation::Activate);

    // Now you can interact with the dynamic body, in this case we're going to
    // give it a velocity.  (note that if we had used CreateBody then we could
    // have set the velocity straight on the body before adding it to the
    // physics system)
    body_interface.SetLinearVelocity(sphere_id, JPH::Vec3(0.0f, -1.0f, 0.0f));

    body_interface.SetLinearVelocity(sphere2_id, JPH::Vec3(0.0f, -1.0f, 0.0f));
    body_interface.SetLinearVelocity(sphere3_id, JPH::Vec3(0.0f, -1.0f, 0.0f));

    // Optional step: Before starting the physics simulation you can optimize
    // the broad phase. This improves collision detection performance (it's
    // pointless here because we only have 2 bodies).  You should definitely
    // not call this every frame or when e.g. streaming in a new level section
    // as it is an expensive operation. Instead insert all new objects in
    // batches instead of 1 at a time to keep the broad phase efficient.
    physics_system.OptimizeBroadPhase();
}

void PhysicsSystem::update(float dt)
{
    // We simulate the physics world in discrete time steps. 60 Hz is a good
    // rate to update the physics system.
    const float cDeltaTime = 1.0f / 60.0f;

    JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();

    // Now we're ready to simulate the body, keep simulating until it goes to sleep
    static std::uint32_t step = 0;

    static glm::vec3 ballPos{};
    static glm::vec3 ball2Pos{};
    static glm::vec3 ball3Pos{};
    if (body_interface.IsActive(sphere_id)) {
        // Next step
        ++step;

        // Output current position and velocity of the sphere
        JPH::RVec3 position = body_interface.GetCenterOfMassPosition(sphere_id);
        ballPos = {position.GetX(), position.GetY(), position.GetZ()};

        JPH::RVec3 position2 = body_interface.GetCenterOfMassPosition(sphere2_id);
        ball2Pos = {position2.GetX(), position2.GetY(), position2.GetZ()};

        JPH::RVec3 position3 = body_interface.GetCenterOfMassPosition(sphere3_id);
        ball3Pos = {position3.GetX(), position3.GetY(), position3.GetZ()};

        JPH::Vec3 velocity = body_interface.GetLinearVelocity(sphere_id);
        std::cout << "Step " << step << ": Position = (" << position.GetX() << ", "
                  << position.GetY() << ", " << position.GetZ() << "), Velocity = ("
                  << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << ")"
                  << std::endl;

        // If you take larger steps than 1 / 60th of a second you need to do
        // multiple collision steps in order to keep the simulation stable. Do
        // 1 collision step per 1 / 60th of a second (round up).
        const int cCollisionSteps = 1;

        // Step the world
        physics_system.Update(cDeltaTime, cCollisionSteps, temp_allocator.get(), &job_system);
    }

    Im3d::PushLayerId(Im3dState::WorldNoDepthLayer);

    Im3d::PushColor(edbr2im3d(RGBColor{255, 0, 255, 128}));
    Im3d::DrawSphereFilled({ballPos.x, ballPos.y, ballPos.z}, 0.5f, 20);
    Im3d::PopColor();

    Im3d::PushColor(edbr2im3d(RGBColor{0, 0, 255, 128}));
    Im3d::DrawSphereFilled({ball2Pos.x, ball2Pos.y, ball2Pos.z}, 0.5f, 20);
    Im3d::PopColor();

    Im3d::PushColor(edbr2im3d(RGBColor{255, 0, 0, 128}));
    Im3d::DrawSphereFilled({ball3Pos.x, ball3Pos.y, ball3Pos.z}, 0.5f, 20);
    Im3d::PopColor();

    Im3d::PopLayerId();
}

void PhysicsSystem::cleanup()
{
    JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();
    // Remove the sphere from the physics system. Note that the sphere itself
    // keeps all of its state and can be re-added at any time.
    body_interface.RemoveBody(sphere_id);

    // Destroy the sphere. After this the sphere ID is no longer valid.
    body_interface.DestroyBody(sphere_id);

    body_interface.RemoveBody(sphere2_id);
    body_interface.DestroyBody(sphere2_id);

    body_interface.RemoveBody(sphere3_id);
    body_interface.DestroyBody(sphere3_id);

    // Remove and destroy the floor
    body_interface.RemoveBody(floor->GetID());
    body_interface.DestroyBody(floor->GetID());

    temp_allocator.reset();

    // Unregisters all types with the factory and cleans up the default material
    JPH::UnregisterTypes();

    // Destroy the factory
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}
