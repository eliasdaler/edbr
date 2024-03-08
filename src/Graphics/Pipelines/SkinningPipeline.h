#pragma once

#include <array>

#include <vulkan/vulkan.h>

#include <Graphics/GfxDevice.h>
#include <Graphics/Vulkan/AppendableBuffer.h>

struct DrawCommand;
class BaseRenderer;

class SkinningPipeline {
public:
    void init(GfxDevice& gfxDevice);
    void cleanup(GfxDevice& gfxDevice);

    void doSkinning(
        VkCommandBuffer cmd,
        std::size_t frameIndex,
        const BaseRenderer& baseRenderer,
        const DrawCommand& dc);

    void beginDrawing(std::size_t frameIndex);
    std::size_t appendJointMatrices(
        std::span<const glm::mat4> jointMatrices,
        std::size_t frameIndex);

private:
    VkPipelineLayout skinningPipelineLayout;
    VkPipeline skinningPipeline;
    struct SkinningPushConstants {
        VkDeviceAddress jointMatricesBuffer;
        std::uint32_t jointMatricesStartIndex;
        std::uint32_t numVertices;
        VkDeviceAddress inputBuffer;
        VkDeviceAddress skinningData;
        VkDeviceAddress outputBuffer;
    };
    static constexpr std::size_t MAX_JOINT_MATRICES = 5000;

    struct PerFrameData {
        AppendableBuffer<glm::mat4> jointMatricesBuffer;
        VkDeviceAddress jointMatricesBufferAddress;
    };

    std::array<PerFrameData, GfxDevice::FRAME_OVERLAP> framesData;

    PerFrameData& getCurrentFrameData(std::size_t frameIndex);
};
