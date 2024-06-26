#include <edbr/Graphics/Pipelines/CRTPipeline.h>

#include <volk.h>

#include <edbr/Graphics/GfxDevice.h>

#include <edbr/Graphics/Vulkan/Pipelines.h>
#include <edbr/Graphics/Vulkan/Util.h>

void CRTPipeline::init(GfxDevice& gfxDevice, VkFormat drawImageFormat)
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
    const auto fragShader = vkutil::loadShaderModule("shaders/crt_lottes.frag.spv", device);
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

void CRTPipeline::cleanup(VkDevice device)
{
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

void CRTPipeline::draw(
    VkCommandBuffer cmd,
    GfxDevice& gfxDevice,
    const GPUImage& inputImage,
    const GPUImage& outputImage)
{
    vkutil::transitionImage(
        cmd, inputImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vkutil::transitionImage(
        cmd, outputImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);

    const auto renderInfo = vkutil::createRenderingInfo({
        .renderExtent = outputImage.getExtent2D(),
        .colorImageView = outputImage.imageView,
    });

    const auto viewport = VkViewport{
        .x = 0,
        .y = 0,
        .width = (float)outputImage.getExtent2D().width,
        .height = (float)outputImage.getExtent2D().height,
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    const auto scissor = VkRect2D{
        .offset = {},
        .extent = outputImage.getExtent2D(),
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    gfxDevice.bindBindlessDescSet(cmd, pipelineLayout);

    const auto pcs = PushConstants{
        .drawImageId = inputImage.getBindlessId(),
        .textureSize = static_cast<glm::vec2>(inputImage.getSize2D()),
    };
    vkCmdPushConstants(
        cmd, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pcs);

    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRendering(cmd);
}
