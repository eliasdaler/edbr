#pragma once

#include <glm/vec3.hpp>

#include <Math/Sphere.h>

#include <Graphics/IdTypes.h>

#include <VkTypes.h>

struct GPUMesh {
    GPUMeshBuffers buffers;
    std::uint32_t numVertices{0};
    std::uint32_t numIndices{0};

    MaterialId materialId{NULL_MATERIAL_ID};

    // AABB
    glm::vec3 minPos;
    glm::vec3 maxPos;
    math::Sphere boundingSphere;

    bool hasSkeleton{false};
    // skinned meshes only
    AllocatedBuffer skinningDataBuffer;
    VkDeviceAddress skinningDataBufferAddress;
};

struct SkinnedMesh {
    AllocatedBuffer skinnedVertexBuffer;
    VkDeviceAddress skinnedVertexBufferAddress;
};
