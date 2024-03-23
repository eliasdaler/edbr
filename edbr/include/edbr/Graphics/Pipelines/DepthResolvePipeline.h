#pragma once

#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

class GfxDevice;
struct GPUImage;

class DepthResolvePipeline {
public:
    void init(GfxDevice& renderer, VkFormat depthImageFormat);
    void cleanup(VkDevice device);

    void draw(
        VkCommandBuffer cmd,
        GfxDevice& gfxDevice,
        const GPUImage& depthImage,
        int numSamples);

private:
    struct PushConstants {
        glm::vec2 depthImageSize;
        std::uint32_t depthImageId;
        std::int32_t numSamples;
    };

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
};
