#include "SkyboxPipeline.h"

#include <volk.h>

#include <Graphics/Camera.h>
#include <Graphics/GfxDevice.h>

#include <Graphics/Vulkan/Pipelines.h>
#include <Graphics/Vulkan/Util.h>

void SkyboxPipeline::init(
    GfxDevice& gfxDevice,
    VkFormat drawImageFormat,
    VkFormat depthImageFormat,
    VkSampleCountFlagBits samples)
{
    const auto& device = gfxDevice.getDevice();

    const auto vertexShader =
        vkutil::loadShaderModule("shaders/fullscreen_triangle.vert.spv", device);
    const auto fragShader = vkutil::loadShaderModule("shaders/skybox.frag.spv", device);

    const auto bindings = std::array<DescriptorLayoutBinding, 1>{{
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
    }};
    skyboxDescSetLayout = vkutil::
        buildDescriptorSetLayout(gfxDevice.getDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, bindings);
    skyboxDescSet = gfxDevice.allocateDescriptorSet(skyboxDescSetLayout);

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(SkyboxPushConstants),
    };

    const auto pushConstantRanges = std::array{bufferRange};
    const auto layouts = std::array{skyboxDescSetLayout};
    pipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);

    pipeline = PipelineBuilder{pipelineLayout}
                   .setShaders(vertexShader, fragShader)
                   .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                   .setPolygonMode(VK_POLYGON_MODE_FILL)
                   .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                   // .setMultisamplingNone()
                   .setMultisampling(samples)
                   .disableBlending()
                   .setColorAttachmentFormat(drawImageFormat)
                   .setDepthFormat(depthImageFormat)
                   // only draw to fragments with depth == 0.0 only
                   .enableDepthTest(false, VK_COMPARE_OP_EQUAL)
                   .build(device);
    vkutil::addDebugLabel(device, pipeline, "skybox pipeline");

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);
}

void SkyboxPipeline::cleanup(VkDevice device)
{
    vkDestroyDescriptorSetLayout(device, skyboxDescSetLayout, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

void SkyboxPipeline::setSkyboxImage(
    const GfxDevice& gfxDevice,
    const AllocatedImage& skybox,
    VkSampler sampler)
{
    DescriptorWriter writer;
    writer.writeImage(
        0,
        skybox.imageView,
        sampler,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.updateSet(gfxDevice.getDevice(), skyboxDescSet);
}

void SkyboxPipeline::draw(VkCommandBuffer cmd, const Camera& camera)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &skyboxDescSet, 0, nullptr);

    const auto pc = SkyboxPushConstants{
        .invViewProj = glm::inverse(camera.getViewProj()),
        .cameraPos = glm::vec4{camera.getPosition(), 1.f},
    };
    vkCmdPushConstants(
        cmd, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyboxPushConstants), &pc);

    vkCmdDraw(cmd, 3, 1, 0, 0);
}
