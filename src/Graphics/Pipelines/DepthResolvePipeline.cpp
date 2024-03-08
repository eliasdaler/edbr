#include "DepthResolvePipeline.h"

#include <array>

#include <Graphics/GfxDevice.h>
#include <Graphics/Vulkan/Descriptors.h>
#include <Graphics/Vulkan/Pipelines.h>
#include <Graphics/Vulkan/Util.h>

void DepthResolvePipeline::init(GfxDevice& gfxDevice, const AllocatedImage& depthImage)
{
    const auto& device = gfxDevice.getDevice();

    const auto drawImageBindings = std::array<DescriptorLayoutBinding, 1>{{
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
    }};
    drawImageDescriptorLayout =
        vkutil::buildDescriptorSetLayout(device, VK_SHADER_STAGE_FRAGMENT_BIT, drawImageBindings);
    drawImageDescriptors = gfxDevice.allocateDescriptorSet(drawImageDescriptorLayout);

    const auto pcRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    const auto pushConstantRanges = std::array{pcRange};
    const auto layouts = std::array{drawImageDescriptorLayout};
    pipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);

    const auto vertexShader =
        vkutil::loadShaderModule("shaders/fullscreen_triangle.vert.spv", device);
    const auto fragShader = vkutil::loadShaderModule("shaders/depth_resolve.frag.spv", device);

    pipeline = PipelineBuilder{pipelineLayout}
                   .setShaders(vertexShader, fragShader)
                   .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                   .setPolygonMode(VK_POLYGON_MODE_FILL)
                   .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                   .setMultisamplingNone()
                   .setDepthFormat(depthImage.format)
                   .enableDepthTest(true, VK_COMPARE_OP_ALWAYS)
                   .build(device);
    vkutil::addDebugLabel(device, pipeline, "depth resolve pipeline");

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);
}

void DepthResolvePipeline::setDepthImage(
    const GfxDevice& gfxDevice,
    const AllocatedImage& depthImage,
    VkSampler nearestSampler)
{
    DescriptorWriter writer;
    writer.writeImage(
        0,
        depthImage.imageView,
        nearestSampler,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.updateSet(gfxDevice.getDevice(), drawImageDescriptors);
}

void DepthResolvePipeline::cleanup(VkDevice device)
{
    vkDestroyDescriptorSetLayout(device, drawImageDescriptorLayout, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
}

void DepthResolvePipeline::draw(VkCommandBuffer cmd, const VkExtent2D& screenSize, int numSamples)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        0,
        1,
        &drawImageDescriptors,
        0,
        nullptr);

    auto pcs = PushConstants{
        .screenSizeAndNumSamples =
            glm::vec4{(float)screenSize.width, (float)screenSize.height, (float)numSamples, 0.f},
    };
    vkCmdPushConstants(
        cmd, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pcs);

    vkCmdDraw(cmd, 3, 1, 0, 0);
}
