#pragma once

#include <array>

#include <vulkan/vulkan.h>

class Renderer;
struct AllocatedImage;

class PostFXPipeline {
public:
    PostFXPipeline(Renderer& renderer, VkFormat drawImageFormat);
    void cleanup(VkDevice device);

    void setDrawImage(const AllocatedImage& drawImage);
    void draw(VkCommandBuffer cmd);

private:
    Renderer& renderer;

    VkPipelineLayout postFXPipelineLayout;
    VkPipeline postFXPipeline;

    VkDescriptorSetLayout postFXDescSetLayout;

    struct FrameData {
        VkDescriptorSet postFXDescSet;
    };
    FrameData& getCurrentFrameData();
    // TODO: Use Renderer::FRAME_OVERLAP
    std::array<FrameData, 2> frames;
};
