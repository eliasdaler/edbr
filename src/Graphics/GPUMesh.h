#pragma once

#include <limits>

#include <glm/vec3.hpp>

#include <Graphics/Material.h>
#include <Math/Sphere.h>

#include <VkTypes.h>

using MeshId = std::size_t;
static const auto NULL_MESH_ID = std::numeric_limits<std::size_t>::max();

struct GPUMesh {
    GPUMeshBuffers buffers;
    std::uint32_t numIndices{0};

    MaterialId materialId{NULL_MATERIAL_ID};

    // AABB
    glm::vec3 minPos;
    glm::vec3 maxPos;
    math::Sphere boundingSphere;

    bool hasSkeleton{false};
};
