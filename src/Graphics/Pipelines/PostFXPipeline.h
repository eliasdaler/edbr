#pragma once

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

    void draw(VkCommandBuffer cmd, const PostFXPushContants& pcs);

    // updating images requires sync (vkDeviceWaitIdle)
    void setImages(const AllocatedImage& drawImage, const AllocatedImage& depthImage);

private:
    Renderer& renderer;

    VkPipelineLayout postFXPipelineLayout;
    VkPipeline postFXPipeline;

    VkDescriptorSetLayout postFXDescSetLayout;
    VkDescriptorSet postFXDescSet;

    PostFXPushContants pcs;
};
