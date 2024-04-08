#pragma once

#include <glm/mat4x4.hpp>

#include <vector>

#include <vulkan/vulkan.h>

class GfxDevice;
class MeshCache;
class Camera;
struct GPUImage;
struct GPUBuffer;
struct MeshDrawCommand;

class MeshPipeline {
public:
    void init(
        GfxDevice& gfxDevice,
        VkFormat drawImageFormat,
        VkFormat depthImageFormat,
        VkSampleCountFlagBits samples);
    void cleanup(VkDevice device);

    void draw(
        VkCommandBuffer cmd,
        VkExtent2D renderExtent,
        const GfxDevice& gfxDevice,
        const MeshCache& meshCache,
        const Camera& camera,
        const GPUBuffer& sceneDataBuffer,
        const std::vector<MeshDrawCommand>& drawCommands,
        const std::vector<std::size_t>& sortedDrawCommands);

private:
    struct PushConstants {
        glm::mat4 transform;
        VkDeviceAddress sceneDataBuffer;
        VkDeviceAddress vertexBuffer;
        std::uint32_t materialId;
        std::uint32_t padding;
    };

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
};
