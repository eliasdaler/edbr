#include <edbr/Graphics/Pipelines/DepthResolvePipeline.h>

#include <array>

#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/Vulkan/Pipelines.h>
#include <edbr/Graphics/Vulkan/Util.h>

void DepthResolvePipeline::init(GfxDevice& gfxDevice, VkFormat depthImageFormat)
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
    const auto fragShader = vkutil::loadShaderModule("shaders/depth_resolve.frag.spv", device);

    pipeline = PipelineBuilder{pipelineLayout}
                   .setShaders(vertexShader, fragShader)
                   .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                   .setPolygonMode(VK_POLYGON_MODE_FILL)
                   .disableCulling()
                   .setMultisamplingNone()
                   .setDepthFormat(depthImageFormat)
                   .enableDepthTest(true, VK_COMPARE_OP_ALWAYS)
                   .build(device);
    vkutil::addDebugLabel(device, pipeline, "depth resolve pipeline");

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);
}

void DepthResolvePipeline::cleanup(VkDevice device)
{
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
}

void DepthResolvePipeline::draw(
    VkCommandBuffer cmd,
    GfxDevice& gfxDevice,
    const GPUImage& depthImage,
    int numSamples)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    gfxDevice.bindBindlessDescSet(cmd, pipelineLayout);

    const auto pcs = PushConstants{
        .depthImageSize =
            {
                (float)depthImage.getExtent2D().width,
                (float)depthImage.getExtent2D().height,
            },
        .depthImageId = depthImage.getBindlessId(),
        .numSamples = numSamples,
    };
    vkCmdPushConstants(
        cmd, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pcs);

    vkCmdDraw(cmd, 3, 1, 0, 0);
}
