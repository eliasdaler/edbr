#pragma once

#include <string>

#include <glm/vec4.hpp>

#include <vulkan/vulkan.h>

#include <Graphics/IdTypes.h>
#include <Graphics/Vulkan/Types.h>

struct MaterialData {
    glm::vec4 baseColor;
    glm::vec4 metalRoughnessEmissive;
    glm::vec4 _padding[14];
};

struct Material {
    glm::vec4 baseColor{1.f, 1.f, 1.f, 1.f};
    float metallicFactor{0.f};
    float roughnessFactor{0.5f};
    float emissiveFactor{0.f};

    ImageId diffuseTexture{NULL_IMAGE_ID};
    ImageId normalMapTexture{NULL_IMAGE_ID};
    ImageId metallicRoughnessTexture{NULL_IMAGE_ID};
    ImageId emissiveTexture{NULL_IMAGE_ID};

    VkDescriptorSet materialSet;

    std::string name;
};
