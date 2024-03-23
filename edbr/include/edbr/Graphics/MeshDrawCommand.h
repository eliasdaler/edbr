#pragma once

#include <glm/mat4x4.hpp>

#include <edbr/Math/Sphere.h>

#include <edbr/Graphics/IdTypes.h>

struct SkinnedMesh;

struct MeshDrawCommand {
    MeshId meshId;
    glm::mat4 transformMatrix;
    math::Sphere worldBoundingSphere;

    const SkinnedMesh* skinnedMesh{nullptr};
    std::uint32_t jointMatricesStartIndex;
    bool castShadow{true};
};
