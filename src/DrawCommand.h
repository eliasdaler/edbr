#pragma once

#include <Math/Sphere.h>
#include <glm/mat4x4.hpp>

struct GPUMesh;

struct DrawCommand {
    const GPUMesh& mesh;
    std::size_t meshId;
    glm::mat4 transformMatrix;
    math::Sphere worldBoundingSphere;
};
