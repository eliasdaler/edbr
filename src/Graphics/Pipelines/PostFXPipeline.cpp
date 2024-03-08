#include "PostFXPipeline.h"

#include <volk.h>

#include <Graphics/GfxDevice.h>

#include <Graphics/Vulkan/Pipelines.h>
#include <Graphics/Vulkan/Util.h>

void PostFXPipeline::init(
    GfxDevice& gfxDevice,
    VkFormat drawImageFormat,
    VkDescriptorSetLayout sceneDataDescriptorLayout)
{
    const auto& device = gfxDevice.getDevice();

    const auto bindings = std::array<DescriptorLayoutBinding, 2>{{
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
    }};
    imagesDescSetLayout = vkutil::
        buildDescriptorSetLayout(gfxDevice.getDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, bindings);
    imagesDescSet = gfxDevice.allocateDescriptorSet(imagesDescSetLayout);

    const auto layouts = std::array{sceneDataDescriptorLayout, imagesDescSetLayout};
    pipelineLayout = vkutil::createPipelineLayout(device, layouts);

    const auto vertexShader =
        vkutil::loadShaderModule("shaders/fullscreen_triangle.vert.spv", device);
    const auto fragShader = vkutil::loadShaderModule("shaders/postfx.frag.spv", device);
    pipeline = PipelineBuilder{pipelineLayout}
                   .setShaders(vertexShader, fragShader)
                   .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                   .setPolygonMode(VK_POLYGON_MODE_FILL)
                   .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
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
    vkDestroyDescriptorSetLayout(device, imagesDescSetLayout, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

void PostFXPipeline::setImages(
    const GfxDevice& gfxDevice,
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
    writer.updateSet(gfxDevice.getDevice(), imagesDescSet);
}

void PostFXPipeline::draw(VkCommandBuffer cmd, VkDescriptorSet sceneDataDescriptorSet)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        0,
        1,
        &sceneDataDescriptorSet,
        0,
        nullptr);
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &imagesDescSet, 0, nullptr);

    vkCmdDraw(cmd, 3, 1, 0, 0);
}
