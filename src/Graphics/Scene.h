#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <Graphics/GPUMesh.h>
#include <Graphics/Material.h>
#include <Graphics/SkeletalAnimation.h>
#include <Graphics/Skeleton.h>
#include <Math/Transform.h>

struct SceneNode {
    std::string name;
    Transform transform;
    std::size_t meshIndex;
    int skinId{-1};

    SceneNode* parent{nullptr};
    std::vector<std::unique_ptr<SceneNode>> children;
};

struct SceneMesh {
    std::vector<MeshId> primitives;
};

struct Scene {
    std::vector<SceneMesh> meshes;
    std::vector<std::unique_ptr<SceneNode>> nodes;
    std::vector<Skeleton> skeletons;
    std::unordered_map<std::string, SkeletalAnimation> animations;
};
