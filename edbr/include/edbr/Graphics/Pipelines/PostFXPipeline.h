#pragma once

#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>

class GfxDevice;
struct GPUImage;
struct GPUBuffer;

class PostFXPipeline {
public:
public:
    void init(GfxDevice& gfxDevice, VkFormat drawImageFormat);
    void cleanup(VkDevice device);

    void draw(
        VkCommandBuffer cmd,
        GfxDevice& gfxDevice,
        const GPUImage& drawImage,
        const GPUImage& depthImage,
        const GPUBuffer& sceneDataBuffer);

private:
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    struct PushConstants {
        VkDeviceAddress sceneDataBuffer;
        std::uint32_t drawImageId;
        std::uint32_t depthImageId;
    };
};
