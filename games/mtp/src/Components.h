#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/mat4x4.hpp>

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

struct MovementComponent {
    glm::vec3 maxSpeed;
    glm::vec3 velocity;
};

struct SkeletonComponent {
    Skeleton skeleton;
    std::vector<SkinnedMesh> skinnedMeshes;
    SkeletonAnimator skeletonAnimator;
    std::unordered_map<std::string, SkeletalAnimation> animations;
};

struct LightComponent {
    Light light;
};

struct TriggerComponent {};

struct PlayerSpawnComponent {};

struct CameraComponent {};

struct PlayerComponent {};
