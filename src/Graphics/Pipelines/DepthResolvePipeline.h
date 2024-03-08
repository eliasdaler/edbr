#pragma once

#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

class GfxDevice;
struct AllocatedImage;

class DepthResolvePipeline {
public:
    void init(GfxDevice& renderer, const AllocatedImage& depthImage);
    void setDepthImage(
        const GfxDevice& renderer,
        const AllocatedImage& depthImage,
        VkSampler nearestSampler);
    void cleanup(VkDevice device);

    void draw(VkCommandBuffer cmd, const VkExtent2D& screenSize, int numSamples);

private:
    struct PushConstants {
        glm::vec4 screenSizeAndNumSamples;
    };

    VkDescriptorSetLayout drawImageDescriptorLayout;
    VkDescriptorSet drawImageDescriptors;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
};
