#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <edbr/Graphics/CPUMesh.h>
#include <edbr/Graphics/Light.h>
#include <edbr/Graphics/Material.h>
#include <edbr/Graphics/SkeletalAnimation.h>
#include <edbr/Graphics/Skeleton.h>
#include <edbr/Math/AABB.h>
#include <edbr/Math/Transform.h>

struct SceneNode {
    std::string name;
    Transform transform;

    int meshIndex{-1}; // index in Scene::meshes
    int skinId{-1}; // index in Scene::skeletons
    int lightId{-1}; // index in Scene::lights
    int cameraId{-1}; // unused for now

    // hierarchy
    SceneNode* parent{nullptr};
    std::vector<std::unique_ptr<SceneNode>> children;
};

struct SceneMesh {
    std::vector<MeshId> primitives;
    std::vector<MaterialId> primitiveMaterials;
};

struct Scene {
    std::filesystem::path path;

    // root nodes
    std::vector<std::unique_ptr<SceneNode>> nodes;

    std::vector<SceneMesh> meshes;
    std::vector<Skeleton> skeletons;
    std::unordered_map<std::string, SkeletalAnimation> animations;
    std::vector<Light> lights;
    std::unordered_map<MeshId, CPUMesh> cpuMeshes;
};

namespace edbr
{
math::AABB calculateBoundingBoxLocal(const Scene& scene, const std::vector<MeshId> meshes);
}
