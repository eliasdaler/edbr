#pragma once

#include <string>

#include <glm/vec4.hpp>

#include <VkTypes.h>

#include <vulkan/vulkan.h>

struct MaterialData {
    glm::vec4 baseColor;
    glm::vec4 metalRoughnessFactors;
    glm::vec4 _padding[14];
};

struct Material {
    glm::vec4 baseColor{1.f, 1.f, 1.f, 1.f};
    float metallicFactor{0.f};
    float roughnessFactor{0.5f};

    bool hasDiffuseTexture{false};

    AllocatedImage diffuseTexture;
    VkDescriptorSet materialSet;

    std::string name;
};
