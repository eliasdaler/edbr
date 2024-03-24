#include <edbr/Graphics/Pipelines/PostFXPipeline.h>

#include <volk.h>

#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/Vulkan/GPUBuffer.h>

#include <edbr/Graphics/Vulkan/Pipelines.h>
#include <edbr/Graphics/Vulkan/Util.h>

void PostFXPipeline::init(GfxDevice& gfxDevice, VkFormat drawImageFormat)
{
    const auto& device = gfxDevice.getDevice();

    const auto pcRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    const auto layouts = std::array{gfxDevice.getBindlessDescSetLayout()};
    const auto pushConstantRanges = std::array{pcRange};
    pipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);

    const auto vertexShader =
        vkutil::loadShaderModule("shaders/fullscreen_triangle.vert.spv", device);
    const auto fragShader = vkutil::loadShaderModule("shaders/postfx.frag.spv", device);
    pipeline = PipelineBuilder{pipelineLayout}
                   .setShaders(vertexShader, fragShader)
                   .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                   .setPolygonMode(VK_POLYGON_MODE_FILL)
                   .disableCulling()
                   .setMultisamplingNone()
                   .disableBlending()
                   .setColorAttachmentFormat(drawImageFormat)
                   .disableDepthTest()
                   .build(device);
    vkutil::addDebugLabel(device, pipeline, "postFX pipeline");

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);
}

void PostFXPipeline::cleanup(VkDevice device)
{
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

void PostFXPipeline::draw(
    VkCommandBuffer cmd,
    GfxDevice& gfxDevice,
    const GPUImage& drawImage,
    const GPUImage& depthImage,
    const GPUBuffer& sceneDataBuffer)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    gfxDevice.bindBindlessDescSet(cmd, pipelineLayout);

    const auto pcs = PushConstants{
        .sceneDataBuffer = sceneDataBuffer.address,
        .drawImageId = drawImage.getBindlessId(),
        .depthImageId = depthImage.getBindlessId(),
    };
    vkCmdPushConstants(
        cmd, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pcs);

    vkCmdDraw(cmd, 3, 1, 0, 0);
}
