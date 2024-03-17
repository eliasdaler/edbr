#pragma once

#include <glm/mat4x4.hpp>

#include <vector>

#include <vulkan/vulkan.h>

class GfxDevice;
class BaseRenderer;
class Camera;
struct AllocatedImage;
struct DrawCommand;

class MeshPipeline {
public:
    void init(
        GfxDevice& gfxDevice,
        VkFormat drawImageFormat,
        VkFormat depthImageFormat,
        VkDescriptorSetLayout bindlessDescSetLayout,
        VkDescriptorSet bindlessDescSet,
        VkDescriptorSetLayout sceneDataDescriptorLayout,
        VkSampleCountFlagBits samples);
    void cleanup(VkDevice device);

    void draw(
        VkCommandBuffer cmd,
        VkExtent2D renderExtent,
        const BaseRenderer& baseRenderer,
        const Camera& camera,
        VkDescriptorSet sceneDataDescriptorSet,
        const std::vector<DrawCommand>& drawCommands,
        const std::vector<std::size_t>& sortedDrawCommands);

private:
    struct PushConstants {
        glm::mat4 transform;
        VkDeviceAddress vertexBuffer;
        std::uint32_t materialId;
    };

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkDescriptorSet bindlessDescSet;
};
