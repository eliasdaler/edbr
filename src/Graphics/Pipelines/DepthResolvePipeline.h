#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <vulkan/vulkan.h>

class Renderer;
struct AllocatedImage;

class DepthResolvePipeline {
public:
    DepthResolvePipeline(Renderer& renderer, const AllocatedImage& depthImage);
    void setDepthImage(
        const Renderer& renderer,
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
