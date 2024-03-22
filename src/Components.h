#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <entt/entt.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <Graphics/GPUMesh.h>
#include <Graphics/Light.h>
#include <Graphics/SkeletalAnimation.h>
#include <Graphics/SkeletonAnimator.h>
#include <Math/Transform.h>

struct TransformComponent {
    Transform transform; // local (relative to parent)
    glm::mat4 worldTransform{1.f};
};

struct HierarchyComponent {
    entt::handle parent{};
    std::vector<entt::handle> children;

    bool hasParent() const
    {
        // would ideally use parent.valid() instead, but enTT segfaults
        // if no registry is set for the handle
        return (bool)parent;
    }
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
