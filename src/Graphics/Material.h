#pragma once

#include <limits>
#include <string>

#include <glm/vec4.hpp>

#include <VkTypes.h>

#include <vulkan/vulkan.h>

struct MaterialData {
    glm::vec4 baseColor;
    glm::vec4 metalRoughnessFactors;
    glm::vec4 _padding[14];
};

using MaterialId = std::size_t;
static const auto NULL_MATERIAL_ID = std::numeric_limits<std::size_t>::max();

struct Material {
    std::string name;

    AllocatedImage diffuseTexture;
    VkDescriptorSet materialSet;

    glm::vec4 baseColor{1.f, 1.f, 1.f, 1.f};
    float metallicFactor{0.f};
    float roughnessFactor{0.5f};

    bool hasDiffuseTexture{false};
};
