#pragma once

#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>

class GfxDevice;
struct DrawableString;
struct AllocatedImage;

class UIDrawingPipeline {
public:
    void init(GfxDevice& gfxDevice, VkFormat drawImageFormat);
    void cleanup(VkDevice device);

    void drawString(
        VkCommandBuffer cmd,
        const AllocatedImage& drawImage,
        const DrawableString& str,
        const glm::mat4& mvp);

    VkDescriptorSetLayout getUiElementDescSetLayout() const { return uiElementDescSetLayout; }

private:
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkPipeline textPipeline;
    VkDescriptorSetLayout uiElementDescSetLayout;

    struct PushConstants {
        glm::mat4 transform;
        glm::vec4 color;
        VkDeviceAddress vertexBuffer;
    };
};
