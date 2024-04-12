#include <edbr/Graphics/Pipelines/SpriteDrawingPipeline.h>

#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/SpriteDrawingCommand.h>
#include <edbr/Graphics/Vulkan/Pipelines.h>
#include <edbr/Graphics/Vulkan/Util.h>
#include <edbr/Math/Transform.h>

#include <glm/gtc/matrix_transform.hpp>

void SpriteDrawingPipeline::init(
    GfxDevice& gfxDevice,
    VkFormat drawImageFormat,
    std::size_t maxSprites)
{
    const auto& device = gfxDevice.getDevice();

    const auto vertexShader = vkutil::loadShaderModule("shaders/sprite.vert.spv", device);
    const auto fragShader = vkutil::loadShaderModule("shaders/sprite.frag.spv", device);

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    const auto layouts = std::array{gfxDevice.getBindlessDescSetLayout()};
    const auto pushConstantRanges = std::array{bufferRange};
    pipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);
    vkutil::addDebugLabel(device, pipelineLayout, "sprite pipeline layout");

    pipeline = PipelineBuilder{pipelineLayout}
                   .setShaders(vertexShader, fragShader)
                   .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                   .setPolygonMode(VK_POLYGON_MODE_FILL)
                   .disableCulling()
                   .setMultisamplingNone()
                   .enableBlending()
                   .setColorAttachmentFormat(drawImageFormat)
                   .disableDepthTest()
                   .build(device);
    vkutil::addDebugLabel(device, pipeline, "UI pipeline");

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);

    for (std::size_t i = 0; i < graphics::FRAME_OVERLAP; ++i) {
        auto& buffer = framesData[i].spriteDrawCommandBuffer;
        buffer = gfxDevice.createBuffer(
            maxSprites * sizeof(SpriteDrawCommand),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        vkutil::addDebugLabel(gfxDevice.getDevice(), buffer.buffer, "sprite draw commands");
    }
}

void SpriteDrawingPipeline::cleanup(GfxDevice& gfxDevice)
{
    for (std::size_t i = 0; i < graphics::FRAME_OVERLAP; ++i) {
        gfxDevice.destroyBuffer(framesData[i].spriteDrawCommandBuffer);
    }
    auto device = gfxDevice.getDevice();
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

void SpriteDrawingPipeline::draw(
    VkCommandBuffer cmd,
    GfxDevice& gfxDevice,
    const GPUImage& drawImage,
    const glm::mat4& viewProj,
    const std::vector<SpriteDrawCommand>& spriteDrawCommands)
{
    TracyVkZoneC(gfxDevice.getTracyVkCtx(), cmd, "Sprite drawing", tracy::Color::Purple);
    if (spriteDrawCommands.empty()) {
        return;
    }

    const auto& commandBuffer =
        getCurrentFrameData(gfxDevice.getCurrentFrameIndex()).spriteDrawCommandBuffer;
    // TODO: maybe do cmd vkCmdCopyBuffer2 here? (will need two cpu side buffers then)
    memcpy(
        commandBuffer.info.pMappedData,
        spriteDrawCommands.data(),
        spriteDrawCommands.size() * sizeof(SpriteDrawCommand));

    const auto renderInfo = vkutil::createRenderingInfo({
        .renderExtent = drawImage.getExtent2D(),
        .colorImageView = drawImage.imageView,
    });

    vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    gfxDevice.bindBindlessDescSet(cmd, pipelineLayout);

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

    const auto pushConstants = PushConstants{
        .viewProj = viewProj,
        .commandsBuffer = commandBuffer.address,
    };
    vkCmdPushConstants(
        cmd,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PushConstants),
        &pushConstants);

    vkCmdDraw(cmd, 6, spriteDrawCommands.size(), 0, 0);

    vkCmdEndRendering(cmd);
}

SpriteDrawingPipeline::PerFrameData& SpriteDrawingPipeline::getCurrentFrameData(
    std::size_t frameIndex)
{
    return framesData[frameIndex];
}
