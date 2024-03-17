#pragma once

#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <Graphics/IdTypes.h>

class GfxDevice;
class Camera;

class SkyboxPipeline {
public:
    void init(
        GfxDevice& gfxDevice,
        VkFormat drawImageFormat,
        VkFormat depthImageFormat,
        VkSampleCountFlagBits samples,
        VkDescriptorSetLayout bindlessDescSetLayout,
        VkDescriptorSet bindlessDescSet);
    void cleanup(VkDevice device);

    void draw(VkCommandBuffer cmd, const Camera& camera);

    void setSkyboxImage(const ImageId skyboxId);

private:
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkDescriptorSet bindlessDescSet;

    ImageId skyboxTextureId;

    struct SkyboxPushConstants {
        glm::mat4 invViewProj;
        glm::vec4 cameraPos;
        std::uint32_t skyboxTextureId;
    };
};
