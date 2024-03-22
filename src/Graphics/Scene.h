#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <Graphics/Light.h>
#include <Graphics/Material.h>
#include <Graphics/SkeletalAnimation.h>
#include <Graphics/Skeleton.h>
#include <Math/Transform.h>

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
};

struct Scene {
    // root nodes
    std::vector<std::unique_ptr<SceneNode>> nodes;

    std::vector<SceneMesh> meshes;
    std::vector<Skeleton> skeletons;
    std::unordered_map<std::string, SkeletalAnimation> animations;
    std::vector<Light> lights;
};
