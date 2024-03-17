#pragma once

#include <array>
#include <vector>

#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>

#include <Graphics/Common.h>
#include <Graphics/IdTypes.h>
#include <Graphics/Vulkan/Types.h>

class GfxDevice;
struct SpriteDrawCommand;

class SpriteDrawingPipeline {
public:
    void init(
        GfxDevice& gfxDevice,
        VkFormat drawImageFormat,
        VkDescriptorSetLayout bindlessDescriptorSetLayout,
        std::size_t maxSprites);
    void cleanup(GfxDevice& gfxDevice);

    void draw(
        VkCommandBuffer cmd,
        VkDescriptorSet bindlessDescSet,
        std::size_t frameIndex,
        const AllocatedImage& drawImage,
        const glm::mat4& viewProj,
        const std::vector<SpriteDrawCommand>& spriteDrawCommands);

private:
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    struct SpriteDrawPushConstants {
        glm::mat4 viewProj;
        VkDeviceAddress commandsBuffer;
        std::uint32_t shaderID;
    };

    struct PerFrameData {
        AllocatedBuffer spriteDrawCommandBuffer;
    };
    std::array<PerFrameData, graphics::FRAME_OVERLAP> framesData;

    PerFrameData& getCurrentFrameData(std::size_t frameIndex);
};
