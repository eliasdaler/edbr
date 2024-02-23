#pragma once

#include <glm/mat4x4.hpp>

#include <Math/Sphere.h>

#include <Graphics/IdTypes.h>

struct SkinnedMesh;

struct DrawCommand {
    MeshId meshId;
    glm::mat4 transformMatrix;
    math::Sphere worldBoundingSphere;
    SkinnedMesh* skinnedMesh{nullptr};
};
