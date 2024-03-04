#pragma once

#include <array>

#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

class Renderer;
struct AllocatedImage;

class PostFXPipeline {
public:
    struct PostFXPushContants {
        glm::mat4 invProj;
        glm::vec4 fogColorAndDensity;
        glm::vec4 ambientColorAndIntensity;
        glm::vec4 sunlightColorAndIntensity;
    };

public:
    PostFXPipeline(Renderer& renderer, VkFormat drawImageFormat);
    void cleanup(VkDevice device);

    void setImages(const AllocatedImage& drawImage, const AllocatedImage& depthImage);
    void draw(VkCommandBuffer cmd, const PostFXPushContants& pcs);

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

    PostFXPushContants pcs;
};
