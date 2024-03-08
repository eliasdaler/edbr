#pragma once

#include <vector>

#include <vulkan/vulkan.h>

class Renderer;
class BaseRenderer;
class Camera;
struct AllocatedImage;
struct DrawCommand;

class MeshPipeline {
public:
    MeshPipeline(
        Renderer& renderer,
        VkFormat drawImageFormat,
        VkFormat depthImageFormat,
        VkDescriptorSetLayout sceneDataDescriptorLayout,
        VkDescriptorSetLayout materialDataDescSetLayout,
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
    VkPipelineLayout meshPipelineLayout;
    VkPipeline meshPipeline;
};
