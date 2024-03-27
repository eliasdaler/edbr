#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <iostream>

namespace Layers
{
static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1) {
        case Layers::NON_MOVING:
            return inObject2 == Layers::MOVING; // Non moving only collides with moving
        case Layers::MOVING:
            return true; // Moving collides with everything
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

namespace BroadPhaseLayers
{
static constexpr JPH::BroadPhaseLayer NON_MOVING{0};
static constexpr JPH::BroadPhaseLayer MOVING{1};
static constexpr std::uint32_t NUM_LAYERS{2};
};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
    BPLayerInterfaceImpl()
    {
        // Create a mapping table from object to broad phase layer
        mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    virtual uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch ((JPH::BroadPhaseLayer::Type)inLayer) {
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
            return "NON_MOVING";
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
            return "MOVING";
        default:
            JPH_ASSERT(false);
            return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1) {
        case Layers::NON_MOVING:
            return inLayer2 == BroadPhaseLayers::MOVING;
        case Layers::MOVING:
            return true;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

// An example contact listener
class MyContactListener : public JPH::ContactListener {
public:
    // See: ContactListener
    JPH::ValidateResult OnContactValidate(
        const JPH::Body& inBody1,
        const JPH::Body& inBody2,
        JPH::RVec3Arg inBaseOffset,
        const JPH::CollideShapeResult& inCollisionResult) override
    {
        std::cout << "Contact validate callback" << std::endl;

        // Allows you to ignore a contact before it is created (using layers to not make objects
        // collide is cheaper!)
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void OnContactAdded(
        const JPH::Body& inBody1,
        const JPH::Body& inBody2,
        const JPH::ContactManifold& inManifold,
        JPH::ContactSettings& ioSettings) override
    {
        std::cout << "A contact was added" << std::endl;
    }

    void OnContactPersisted(
        const JPH::Body& inBody1,
        const JPH::Body& inBody2,
        const JPH::ContactManifold& inManifold,
        JPH::ContactSettings& ioSettings) override
    {
        std::cout << "A contact was persisted" << std::endl;
    }

    void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
    {
        std::cout << "A contact was removed" << std::endl;
    }
};

// An example activation listener
class MyBodyActivationListener : public JPH::BodyActivationListener {
public:
    void OnBodyActivated(const JPH::BodyID& inBodyID, std::uint64_t inBodyUserData) override
    {
        std::cout << "A body got activated" << std::endl;
    }

    void OnBodyDeactivated(const JPH::BodyID& inBodyID, std::uint64_t inBodyUserData) override
    {
        std::cout << "A body went to sleep" << std::endl;
    }
};

class PhysicsSystem {
public:
    void init();
    void update(float dt);
    void cleanup();

private:
    JPH::PhysicsSystem physics_system;
    JPH::BodyID sphere_id;
    JPH::BodyID sphere2_id;
    JPH::BodyID sphere3_id;
    JPH::Body* floor{nullptr};

    std::unique_ptr<JPH::TempAllocatorImpl> temp_allocator;
    JPH::JobSystemThreadPool job_system;

    // Create mapping table from object layer to broadphase layer
    BPLayerInterfaceImpl broad_phase_layer_interface;
    // Filters object vs broadphase layers
    ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
    // Filters object vs object layers
    ObjectLayerPairFilterImpl object_vs_object_layer_filter;

    MyContactListener contact_listener;
    MyBodyActivationListener body_activation_listener;
};
