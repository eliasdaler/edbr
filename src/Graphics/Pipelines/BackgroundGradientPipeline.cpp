#include "BackgroundGradientPipeline.h"

#include <array>

#include <Graphics/Renderer.h>
#include <Graphics/Vulkan/Descriptors.h>
#include <Graphics/Vulkan/Pipelines.h>
#include <Graphics/Vulkan/Util.h>

void BackgroundGradientPipeline::init(Renderer& renderer, const AllocatedImage& drawImage)
{
    const auto& device = renderer.getDevice();

    const auto drawImageBindings = std::array<DescriptorLayoutBinding, 1>{
        {0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
    };
    drawImageDescriptorLayout =
        vkutil::buildDescriptorSetLayout(device, VK_SHADER_STAGE_COMPUTE_BIT, drawImageBindings);
    drawImageDescriptors = renderer.allocateDescriptorSet(drawImageDescriptorLayout);

    { // write descriptors
        DescriptorWriter writer;
        writer.writeImage(
            0,
            drawImage.imageView,
            VK_NULL_HANDLE,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.updateSet(device, drawImageDescriptors);
    }

    const auto pushConstant = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(ComputePushConstants),
    };

    const auto pushConstants = std::array{pushConstant};
    const auto layouts = std::array{drawImageDescriptorLayout};
    gradientPipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstants);

    const auto shader = vkutil::loadShaderModule("shaders/gradient.comp.spv", device);
    vkutil::addDebugLabel(device, shader, "gradient");

    gradientPipeline =
        ComputePipelineBuilder{gradientPipelineLayout}.setShader(shader).build(device);

    vkDestroyShaderModule(device, shader, nullptr);

    gradientConstants = ComputePushConstants{
        .data1 = glm::vec4{239.f / 255.f, 157.f / 255.f, 8.f / 255.f, 1},
        .data2 = glm::vec4{85.f / 255.f, 18.f / 255.f, 85.f / 255.f, 1},
    };
}

void BackgroundGradientPipeline::cleanup(VkDevice device)
{
    vkDestroyDescriptorSetLayout(device, drawImageDescriptorLayout, nullptr);
    vkDestroyPipelineLayout(device, gradientPipelineLayout, nullptr);
    vkDestroyPipeline(device, gradientPipeline, nullptr);
}

void BackgroundGradientPipeline::draw(VkCommandBuffer cmd, const glm::vec2& imageSize)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradientPipeline);
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        gradientPipelineLayout,
        0,
        1,
        &drawImageDescriptors,
        0,
        nullptr);

    vkCmdPushConstants(
        cmd,
        gradientPipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(ComputePushConstants),
        &gradientConstants);

    vkCmdDispatch(cmd, std::ceil(imageSize.x / 16.f), std::ceil(imageSize.y / 16.f), 1.f);
}
