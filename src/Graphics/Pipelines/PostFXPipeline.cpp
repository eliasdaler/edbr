#include "PostFXPipeline.h"

#include <volk.h>

#include <Graphics/Renderer.h>

#include <Graphics/Vulkan/Pipelines.h>
#include <Graphics/Vulkan/Util.h>

PostFXPipeline::PostFXPipeline(Renderer& renderer, VkFormat drawImageFormat) : renderer(renderer)
{
    const auto& device = renderer.getDevice();

    const auto vertexShader =
        vkutil::loadShaderModule("shaders/fullscreen_triangle.vert.spv", device);
    const auto fragShader = vkutil::loadShaderModule("shaders/postfx.frag.spv", device);

    const auto bindings = std::array<DescriptorLayoutBinding, 1>{{
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
    }};
    postFXDescSetLayout = vkutil::
        buildDescriptorSetLayout(renderer.getDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, bindings);
    for (auto& fd : frames) {
        fd.postFXDescSet = renderer.allocateDescriptorSet(postFXDescSetLayout);
    }

    const auto layouts = std::array{postFXDescSetLayout};
    postFXPipelineLayout = vkutil::createPipelineLayout(device, layouts);

    postFXPipeline = PipelineBuilder{postFXPipelineLayout}
                         .setShaders(vertexShader, fragShader)
                         .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                         .setPolygonMode(VK_POLYGON_MODE_FILL)
                         .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                         .setMultisamplingNone()
                         .disableBlending()
                         .setColorAttachmentFormat(drawImageFormat)
                         .disableDepthTest()
                         .build(device);
    vkutil::addDebugLabel(device, postFXPipeline, "postFX pipeline");

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);
}

void PostFXPipeline::cleanup(VkDevice device)
{
    vkDestroyDescriptorSetLayout(device, postFXDescSetLayout, nullptr);
    vkDestroyPipeline(device, postFXPipeline, nullptr);
    vkDestroyPipelineLayout(device, postFXPipelineLayout, nullptr);
}

PostFXPipeline::FrameData& PostFXPipeline::getCurrentFrameData()
{
    return frames[renderer.getCurrentFrameIndex()];
}

void PostFXPipeline::setDrawImage(const AllocatedImage& drawImage)
{
    DescriptorWriter writer;
    writer.writeImage(
        0,
        drawImage.imageView,
        renderer.getDefaultNearestSampler(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.updateSet(renderer.getDevice(), getCurrentFrameData().postFXDescSet);
}

void PostFXPipeline::draw(VkCommandBuffer cmd)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, postFXPipeline);
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        postFXPipelineLayout,
        0,
        1,
        &getCurrentFrameData().postFXDescSet,
        0,
        nullptr);

    vkCmdDraw(cmd, 3, 1, 0, 0);
}
