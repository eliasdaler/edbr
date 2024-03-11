#pragma once

#include <string>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include <Math/Rect.h>

#include <Graphics/Vulkan/Types.h>

struct Font;
class GfxDevice;

struct DrawableString {
    void init(
        GfxDevice& gfxDevice,
        std::string text,
        const Font& font,
        VkSampler linearSampler,
        VkDescriptorSetLayout uiElementDescSetLayout);
    void cleanup(const GfxDevice& gfxDevice);

    glm::vec2 position;
    float scale{1.f};

    const Font* font{nullptr};
    std::string text;

    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;
    std::uint32_t numIndices{0};

    VkDescriptorSet uiElementDescSet;

    bool initialized{false};

    glm::vec4 color{1.f, 1.f, 1.f, 1.f}; // white
};
