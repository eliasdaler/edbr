#include "UIDrawingPipeline.h"

#include <Graphics/DrawableString.h>
#include <Graphics/GfxDevice.h>
#include <Graphics/Vulkan/Pipelines.h>
#include <Graphics/Vulkan/Util.h>

void UIDrawingPipeline::init(GfxDevice& gfxDevice, VkFormat drawImageFormat)
{
    const auto& device = gfxDevice.getDevice();

    const auto uiElementBindings = std::array<DescriptorLayoutBinding, 1>{{
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
    }};
    uiElementDescSetLayout = vkutil::buildDescriptorSetLayout(
        gfxDevice.getDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, uiElementBindings);

    const auto vertexShader = vkutil::loadShaderModule("shaders/ui_element.vert.spv", device);
    const auto fragShader = vkutil::loadShaderModule("shaders/ui_element.frag.spv", device);

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    const auto layouts = std::array{uiElementDescSetLayout};
    const auto pushConstantRanges = std::array{bufferRange};
    pipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);
    vkutil::addDebugLabel(device, pipelineLayout, "UI pipeline layout");

    pipeline = PipelineBuilder{pipelineLayout}
                   .setShaders(vertexShader, fragShader)
                   .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                   .setPolygonMode(VK_POLYGON_MODE_FILL)
                   .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                   .setMultisamplingNone()
                   .enableBlending()
                   .setColorAttachmentFormat(drawImageFormat)
                   .disableDepthTest()
                   .build(device);
    vkutil::addDebugLabel(device, pipeline, "UI pipeline");

    const auto textFragShader = vkutil::loadShaderModule("shaders/ui_text.frag.spv", device);
    textPipeline = PipelineBuilder{pipelineLayout}
                       .setShaders(vertexShader, textFragShader)
                       .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                       .setPolygonMode(VK_POLYGON_MODE_FILL)
                       .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                       .setMultisamplingNone()
                       .enableBlending()
                       .setColorAttachmentFormat(drawImageFormat)
                       .disableDepthTest()
                       .build(device);
    vkutil::addDebugLabel(device, textPipeline, "UI pipeline (text)");

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);
    vkDestroyShaderModule(device, textFragShader, nullptr);
}

void UIDrawingPipeline::cleanup(VkDevice device)
{
    vkDestroyDescriptorSetLayout(device, uiElementDescSetLayout, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipeline(device, textPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

void UIDrawingPipeline::drawString(
    VkCommandBuffer cmd,
    const AllocatedImage& drawImage,
    const DrawableString& str,
    const glm::mat4& mvp)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, textPipeline);

    const auto renderInfo = vkutil::createRenderingInfo({
        .renderExtent = drawImage.getExtent2D(),
        .colorImageView = drawImage.imageView,
    });

    vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

    const auto viewport = VkViewport{
        .x = 0,
        .y = 0,
        .width = (float)drawImage.getExtent2D().width,
        .height = (float)drawImage.getExtent2D().height,
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    const auto scissor = VkRect2D{
        .offset = {},
        .extent = drawImage.getExtent2D(),
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        0,
        1,
        &str.uiElementDescSet,
        0,
        nullptr);

    const auto pushConstants = PushConstants{
        .transform = mvp,
        .color = str.color,
        .vertexBuffer = str.vertexBuffer.address,
    };
    vkCmdPushConstants(
        cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

    vkCmdBindIndexBuffer(cmd, str.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, str.numIndices, 1, 0, 0, 0);

    vkCmdEndRendering(cmd);
}
