#pragma once

#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

class Renderer;
struct AllocatedImage;
class Camera;

class SkyboxPipeline {
public:
    SkyboxPipeline(
        Renderer& renderer,
        VkFormat drawImageFormat,
        VkFormat depthImageFormat,
        VkSampleCountFlagBits samples);
    void cleanup(VkDevice device);

    void draw(VkCommandBuffer cmd, const Camera& camera);

    // updating the image requires sync (vkDeviceWaitIdle)
    void setSkyboxImage(const AllocatedImage& skybox);

private:
    Renderer& renderer;

    VkPipelineLayout skyboxPipelineLayout;
    VkPipeline skyboxPipeline;

    VkDescriptorSetLayout skyboxDescSetLayout;
    VkDescriptorSet skyboxDescSet;

    struct SkyboxPushConstants {
        glm::mat4 invViewProj;
        glm::vec4 cameraPos;
    };
};
