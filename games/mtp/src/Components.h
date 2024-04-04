#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/mat4x4.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

#include <edbr/Graphics/GPUMesh.h>
#include <edbr/Graphics/Light.h>
#include <edbr/Graphics/SkeletalAnimation.h>
#include <edbr/Graphics/SkeletonAnimator.h>
#include <edbr/Math/Transform.h>

struct TransformComponent {
    Transform transform; // local (relative to parent)
    glm::mat4 worldTransform{1.f};
};

struct TagComponent {
    friend class Game;

public:
    // call Game::setEntityTag to set the tag
    const std::string& getTag() const { return tag; }

private:
    void setTag(const std::string& t) { tag = t; }

    std::string tag;
};

struct MeshComponent {
    std::vector<MeshId> meshes;
    std::vector<Transform> meshTransforms;
    std::filesystem::path meshPath; // path to gltf scene from which model is loaded
};

struct ColliderComponent {};

struct MovementComponent {
    glm::vec3 kinematicVelocity; // manual velocity for kinematic objects
    glm::vec3 maxSpeed; // only for kinematic speed

    glm::vec3 prevFramePosition;
    glm::vec3 effectiveVelocity;

    // smooth rotation
    glm::quat startHeading;
    glm::quat targetHeading;
    float rotationProgress{0.f}; // from [0 to rotationTime] (used for slerp)
    float rotationTime{0.f};
};

struct SkeletonComponent {
    Skeleton skeleton;
    std::vector<SkinnedMesh> skinnedMeshes;
    SkeletonAnimator skeletonAnimator;

    // pointer to the animations stored in SkeletalAnimationCache
    const std::unordered_map<std::string, SkeletalAnimation>* animations{nullptr};
};

struct LightComponent {
    Light light;
};

struct TriggerComponent {};

struct PlayerSpawnComponent {};

struct CameraComponent {};

struct PlayerComponent {};

struct PhysicsComponent {
    JPH::BodyID bodyId;
};

struct AnimationEventSoundComponent {
    const std::string& getSoundName(const std::string& eventName) const
    {
        if (auto it = eventSounds.find(eventName); it != eventSounds.end()) {
            return it->second;
        }
        static const std::string emptyString{};
        return emptyString;
    }

    // event name -> sound name
    std::unordered_map<std::string, std::string> eventSounds;
};