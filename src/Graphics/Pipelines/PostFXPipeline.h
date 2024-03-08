#pragma once

#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

class GfxDevice;
struct AllocatedImage;

class PostFXPipeline {
public:
public:
    void init(
        GfxDevice& renderer,
        VkFormat drawImageFormat,
        VkDescriptorSetLayout sceneDataDescriptorLayout);
    void cleanup(VkDevice device);

    void draw(VkCommandBuffer cmd, VkDescriptorSet sceneDataDescriptorSet);

    // updating images requires sync (vkDeviceWaitIdle)
    void setImages(
        const GfxDevice& gfxDevice,
        const AllocatedImage& drawImage,
        const AllocatedImage& depthImage,
        VkSampler nearestSampler);

private:
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkDescriptorSetLayout imagesDescSetLayout;
    VkDescriptorSet imagesDescSet;
};
