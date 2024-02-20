#pragma once

#include <limits>
#include <string>

#include <glm/vec4.hpp>

#include <Graphics/Texture.h>

struct MaterialData {
    glm::vec4 baseColor;
};

using MaterialId = std::size_t;
static const auto NULL_MATERIAL_ID = std::numeric_limits<std::size_t>::max();

struct Material {
    std::string name;

    // wgpu::Buffer dataBuffer;
    Texture diffuseTexture;
    glm::vec4 baseColor{1.f, 1.f, 1.f, 1.f};

    // wgpu::BindGroup bindGroup;
};
