#include "SkinningPipeline.h"

#include <Graphics/Vulkan/Pipelines.h>
#include <Graphics/Vulkan/Util.h>

#include <Graphics/DrawCommand.h>

SkinningPipeline::SkinningPipeline(Renderer& renderer) : renderer(renderer)
{
    const auto& device = renderer.getDevice();

    const auto pushConstant = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(SkinningPushConstants),
    };

    const auto pushConstants = std::array{pushConstant};
    skinningPipelineLayout = vkutil::createPipelineLayout(device, {}, pushConstants);

    const auto shader = vkutil::loadShaderModule("shaders/skinning.comp.spv", device);
    vkutil::addDebugLabel(device, shader, "skinning");

    skinningPipeline =
        ComputePipelineBuilder{skinningPipelineLayout}.setShader(shader).build(device);

    vkDestroyShaderModule(device, shader, nullptr);

    for (std::size_t i = 0; i < Renderer::FRAME_OVERLAP; ++i) {
        auto& jointMatricesBuffer = framesData[i].jointMatricesBuffer;
        jointMatricesBuffer.capacity = MAX_JOINT_MATRICES;
        jointMatricesBuffer.buffer = renderer.createBuffer(
            MAX_JOINT_MATRICES * sizeof(glm::mat4),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);
        framesData[i].jointMatricesBufferAddress =
            renderer.getBufferAddress(jointMatricesBuffer.buffer);
    }
}

void SkinningPipeline::cleanup(VkDevice device)
{
    for (std::size_t i = 0; i < Renderer::FRAME_OVERLAP; ++i) {
        renderer.destroyBuffer(framesData[i].jointMatricesBuffer.buffer);
    }
    vkDestroyPipelineLayout(device, skinningPipelineLayout, nullptr);
    vkDestroyPipeline(device, skinningPipeline, nullptr);
}

void SkinningPipeline::beginDrawing()
{
    getCurrentFrameData().jointMatricesBuffer.clear();
}

SkinningPipeline::PerFrameData& SkinningPipeline::getCurrentFrameData()
{
    return framesData[renderer.getCurrentFrameIndex()];
}

std::size_t SkinningPipeline::appendJointMatrices(std::span<const glm::mat4> jointMatrices)
{
    auto& jointMatricesBuffer = getCurrentFrameData().jointMatricesBuffer;
    const auto startIndex = jointMatricesBuffer.size;
    jointMatricesBuffer.append(jointMatrices);
    return startIndex;
}

void SkinningPipeline::doSkinning(VkCommandBuffer cmd, const DrawCommand& dc)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, skinningPipeline);

    const auto& mesh = renderer.getMesh(dc.meshId);
    assert(mesh.hasSkeleton);
    assert(dc.skinnedMesh);

    const auto cs = SkinningPushConstants{
        .jointMatricesBuffer = getCurrentFrameData().jointMatricesBufferAddress,
        .jointMatricesStartIndex = dc.jointMatricesStartIndex,
        .numVertices = mesh.numVertices,
        .inputBuffer = mesh.buffers.vertexBufferAddress,
        .skinningData = mesh.skinningDataBufferAddress,
        .outputBuffer = dc.skinnedMesh->skinnedVertexBufferAddress,
    };
    vkCmdPushConstants(
        cmd,
        skinningPipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(SkinningPushConstants),
        &cs);

    static const auto workgroupSize = 256;
    vkCmdDispatch(cmd, std::ceil(mesh.numVertices / (float)workgroupSize), 1, 1);
}
