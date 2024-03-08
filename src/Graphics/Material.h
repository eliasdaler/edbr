#pragma once

#include <string>

#include <glm/vec4.hpp>

#include <vulkan/vulkan.h>

#include <Graphics/IdTypes.h>
#include <Graphics/Vulkan/Types.h>

struct MaterialData {
    glm::vec4 baseColor;
    glm::vec4 metalRoughnessFactors;
    glm::vec4 _padding[14];
};

struct Material {
    glm::vec4 baseColor{1.f, 1.f, 1.f, 1.f};

    ImageId diffuseTexture{NULL_IMAGE_ID};

    float metallicFactor{0.f};
    float roughnessFactor{0.5f};

    VkDescriptorSet materialSet;

    std::string name;
};
