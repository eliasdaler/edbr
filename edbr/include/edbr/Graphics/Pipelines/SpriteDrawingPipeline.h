#pragma once

#include <array>
#include <vector>

#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>

#include <edbr/Graphics/Common.h>
#include <edbr/Graphics/IdTypes.h>
#include <edbr/Graphics/Vulkan/GPUBuffer.h>
#include <edbr/Graphics/Vulkan/GPUImage.h>

class GfxDevice;
struct SpriteDrawCommand;
struct GPUImage;

class SpriteDrawingPipeline {
public:
    void init(GfxDevice& gfxDevice, VkFormat drawImageFormat, std::size_t maxSprites);
    void cleanup(GfxDevice& gfxDevice);

    void draw(
        VkCommandBuffer cmd,
        GfxDevice& gfxDevice,
        const GPUImage& drawImage,
        const glm::mat4& viewProj,
        const std::vector<SpriteDrawCommand>& spriteDrawCommands);

private:
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    struct PushConstants {
        glm::mat4 viewProj;
        VkDeviceAddress commandsBuffer;
    };

    struct PerFrameData {
        GPUBuffer spriteDrawCommandBuffer;
    };
    std::array<PerFrameData, graphics::FRAME_OVERLAP> framesData;

    PerFrameData& getCurrentFrameData(std::size_t frameIndex);
};
