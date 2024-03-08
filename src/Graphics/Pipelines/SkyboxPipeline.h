#pragma once

#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

class GfxDevice;
struct AllocatedImage;
class Camera;

class SkyboxPipeline {
public:
    void init(
        GfxDevice& gfxDevice,
        VkFormat drawImageFormat,
        VkFormat depthImageFormat,
        VkSampleCountFlagBits samples);
    void cleanup(VkDevice device);

    void draw(VkCommandBuffer cmd, const Camera& camera);

    // updating the image requires sync (vkDeviceWaitIdle)
    void setSkyboxImage(
        const GfxDevice& gfxDevice,
        const AllocatedImage& skybox,
        VkSampler sampler);

private:
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkDescriptorSetLayout skyboxDescSetLayout;
    VkDescriptorSet skyboxDescSet;

    struct SkyboxPushConstants {
        glm::mat4 invViewProj;
        glm::vec4 cameraPos;
    };
};
