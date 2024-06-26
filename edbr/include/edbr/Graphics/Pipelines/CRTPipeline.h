#pragma once

#include <vulkan/vulkan.h>

class GfxDevice;
struct GPUImage;

#include <glm/vec2.hpp>

class CRTPipeline {
public:
    void init(GfxDevice& gfxDevice, VkFormat drawImageFormat);
    void cleanup(VkDevice device);

    void draw(
        VkCommandBuffer cmd,
        GfxDevice& gfxDevice,
        const GPUImage& inputImage,
        const GPUImage& outputImage);

private:
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    struct PushConstants {
        std::uint32_t drawImageId;
        float gamma{2.2};
        glm::vec2 textureSize;
    };
};
