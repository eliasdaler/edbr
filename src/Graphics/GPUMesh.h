#pragma once

#include <glm/vec3.hpp>

#include <Math/Sphere.h>

#include <Graphics/IdTypes.h>
#include <Graphics/Vulkan/Types.h>

struct GPUMesh {
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;

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
};

struct SkinnedMesh {
    AllocatedBuffer skinnedVertexBuffer;
};
