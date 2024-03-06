#pragma once

#include <vector>

#include <vulkan/vulkan.h>

class Renderer;
class Camera;
struct AllocatedImage;
struct DrawCommand;

class MeshPipeline {
public:
    MeshPipeline(
        Renderer& renderer,
        VkFormat drawImageFormat,
        VkFormat depthImageFormat,
        VkSampleCountFlagBits samples);
    void cleanup(VkDevice device);

    void draw(
        VkCommandBuffer cmd,
        VkExtent2D renderExtent,
        const Camera& camera,
        VkDescriptorSet sceneDataDescriptorSet,
        const std::vector<DrawCommand>& drawCommands,
        const std::vector<std::size_t>& sortedDrawCommands);

private:
    Renderer& renderer;

    VkPipelineLayout meshPipelineLayout;
    VkPipeline meshPipeline;
};
