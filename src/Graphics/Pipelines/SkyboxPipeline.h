#pragma once

#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

class Renderer;
class AllocatedImage;
class Camera;

class SkyboxPipeline {
public:
    SkyboxPipeline(Renderer& renderer, VkFormat drawImageFormat, VkFormat depthImageFormat);
    void cleanup(VkDevice device);

    void setSkyboxImage(const AllocatedImage& skybox);
    void draw(VkCommandBuffer cmd, const Camera& camera);

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
