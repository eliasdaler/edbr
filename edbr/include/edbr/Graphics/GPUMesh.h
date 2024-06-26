#pragma once

#include <glm/vec3.hpp>

#include <edbr/Math/Sphere.h>

#include <edbr/Graphics/IdTypes.h>
#include <edbr/Graphics/Vulkan/GPUBuffer.h>

struct GPUMesh {
    GPUBuffer vertexBuffer;
    GPUBuffer indexBuffer;

    std::uint32_t numVertices{0};
    std::uint32_t numIndices{0};

    // AABB
    glm::vec3 minPos;
    glm::vec3 maxPos;
    math::Sphere boundingSphere;

    bool hasSkeleton{false};
    // skinned meshes only
    GPUBuffer skinningDataBuffer;
};

struct SkinnedMesh {
    GPUBuffer skinnedVertexBuffer;
};
