#include <edbr/Graphics/Pipelines/SkinningPipeline.h>

#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/MeshCache.h>
#include <edbr/Graphics/MeshDrawCommand.h>
#include <edbr/Graphics/Vulkan/Pipelines.h>
#include <edbr/Graphics/Vulkan/Util.h>

void SkinningPipeline::init(GfxDevice& gfxDevice)
{
    const auto& device = gfxDevice.getDevice();

    const auto pushConstant = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    const auto pushConstants = std::array{pushConstant};
    skinningPipelineLayout = vkutil::createPipelineLayout(device, {}, pushConstants);

    const auto shader = vkutil::loadShaderModule("shaders/skinning.comp.spv", device);
    vkutil::addDebugLabel(device, shader, "skinning");

    skinningPipeline =
        ComputePipelineBuilder{skinningPipelineLayout}.setShader(shader).build(device);

    vkDestroyShaderModule(device, shader, nullptr);

    for (std::size_t i = 0; i < graphics::FRAME_OVERLAP; ++i) {
        auto& jointMatricesBuffer = framesData[i].jointMatricesBuffer;
        jointMatricesBuffer.capacity = MAX_JOINT_MATRICES;
        jointMatricesBuffer.buffer = gfxDevice.createBuffer(
            MAX_JOINT_MATRICES * sizeof(glm::mat4),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    }
}

void SkinningPipeline::cleanup(GfxDevice& gfxDevice)
{
    for (std::size_t i = 0; i < graphics::FRAME_OVERLAP; ++i) {
        gfxDevice.destroyBuffer(framesData[i].jointMatricesBuffer.buffer);
    }
    vkDestroyPipelineLayout(gfxDevice.getDevice(), skinningPipelineLayout, nullptr);
    vkDestroyPipeline(gfxDevice.getDevice(), skinningPipeline, nullptr);
}

void SkinningPipeline::beginDrawing(std::size_t frameIndex)
{
    getCurrentFrameData(frameIndex).jointMatricesBuffer.clear();
}

SkinningPipeline::PerFrameData& SkinningPipeline::getCurrentFrameData(std::size_t frameIndex)
{
    return framesData[frameIndex];
}

std::size_t SkinningPipeline::appendJointMatrices(
    std::span<const glm::mat4> jointMatrices,
    std::size_t frameIndex)
{
    auto& jointMatricesBuffer = getCurrentFrameData(frameIndex).jointMatricesBuffer;
    const auto startIndex = jointMatricesBuffer.size;
    jointMatricesBuffer.append(jointMatrices);
    return startIndex;
}

void SkinningPipeline::doSkinning(
    VkCommandBuffer cmd,
    std::size_t frameIndex,
    const MeshCache& meshCache,
    const MeshDrawCommand& dc)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, skinningPipeline);

    const auto& mesh = meshCache.getMesh(dc.meshId);
    assert(mesh.hasSkeleton);
    assert(dc.skinnedMesh);

    const auto cs = PushConstants{
        .jointMatricesBuffer = getCurrentFrameData(frameIndex).jointMatricesBuffer.buffer.address,
        .jointMatricesStartIndex = dc.jointMatricesStartIndex,
        .numVertices = mesh.numVertices,
        .inputBuffer = mesh.vertexBuffer.address,
        .skinningData = mesh.skinningDataBuffer.address,
        .outputBuffer = dc.skinnedMesh->skinnedVertexBuffer.address,
    };
    vkCmdPushConstants(
        cmd, skinningPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &cs);

    static const auto workgroupSize = 256;
    const auto groupSizeX = (std::uint32_t)std::ceil(mesh.numVertices / (float)workgroupSize);
    vkCmdDispatch(cmd, groupSizeX, 1, 1);
}
