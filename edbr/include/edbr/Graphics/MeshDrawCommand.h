#pragma once

#include <glm/mat4x4.hpp>

#include <edbr/Math/Sphere.h>

#include <edbr/Graphics/IdTypes.h>

struct SkinnedMesh;

struct MeshDrawCommand {
    MeshId meshId;
    glm::mat4 transformMatrix;

    // for frustum culling
    math::Sphere worldBoundingSphere;

    // If set - mesh will be drawn with overrideMaterialId
    // instead of whatever material the mesh has
    MaterialId materialId{NULL_MATERIAL_ID};

    bool castShadow{true};

    // skinned meshes only
    const SkinnedMesh* skinnedMesh{nullptr};
    std::uint32_t jointMatricesStartIndex;
};
