#pragma once

#include <array>

#include <vulkan/vulkan.h>

#include <Graphics/Renderer.h>
#include <Graphics/Vulkan/AppendableBuffer.h>

struct DrawCommand;

class SkinningPipeline {
public:
    SkinningPipeline(Renderer& renderer);

    void cleanup(VkDevice device);

    void doSkinning(VkCommandBuffer cmd, const DrawCommand& dc);

    void beginDrawing();
    std::size_t appendJointMatrices(std::span<const glm::mat4> jointMatrices);

private:
    Renderer& renderer;

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

    std::array<PerFrameData, Renderer::FRAME_OVERLAP> framesData;

    PerFrameData& getCurrentFrameData();
};
