#include "PostFXPipeline.h"

#include <volk.h>

#include <Graphics/Renderer.h>

#include <Graphics/Vulkan/Pipelines.h>
#include <Graphics/Vulkan/Util.h>

PostFXPipeline::PostFXPipeline(Renderer& renderer, VkFormat drawImageFormat) : renderer(renderer)
{
    const auto& device = renderer.getDevice();

    const auto bindings = std::array<DescriptorLayoutBinding, 2>{{
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
    }};
    postFXDescSetLayout = vkutil::
        buildDescriptorSetLayout(renderer.getDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, bindings);
    postFXDescSet = renderer.allocateDescriptorSet(postFXDescSetLayout);

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PostFXPushContants),
    };

    const auto pushConstantRanges = std::array{bufferRange};
    const auto layouts = std::array{postFXDescSetLayout};
    postFXPipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);

    const auto vertexShader =
        vkutil::loadShaderModule("shaders/fullscreen_triangle.vert.spv", device);
    const auto fragShader = vkutil::loadShaderModule("shaders/postfx.frag.spv", device);
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

void PostFXPipeline::setImages(
    const AllocatedImage& drawImage,
    const AllocatedImage& depthImage,
    VkSampler nearestSampler)
{
    DescriptorWriter writer;
    writer.writeImage(
        0,
        drawImage.imageView,
        nearestSampler,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.writeImage(
        1,
        depthImage.imageView,
        nearestSampler,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.updateSet(renderer.getDevice(), postFXDescSet);
}

void PostFXPipeline::draw(VkCommandBuffer cmd, const PostFXPushContants& pcs)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, postFXPipeline);
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        postFXPipelineLayout,
        0,
        1,
        &postFXDescSet,
        0,
        nullptr);

    vkCmdPushConstants(
        cmd,
        postFXPipelineLayout,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PostFXPushContants),
        &pcs);

    vkCmdDraw(cmd, 3, 1, 0, 0);
}
