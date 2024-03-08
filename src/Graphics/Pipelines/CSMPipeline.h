#pragma once

#include <array>
#include <vector>

#include <Graphics/Camera.h>

#include <Graphics/Vulkan/Types.h>

class GfxDevice;
class BaseRenderer;
struct DrawCommand;

class CSMPipeline {
public:
    static const int NUM_SHADOW_CASCADES = 3;

public:
    void init(GfxDevice& gfxDevice, const std::array<float, NUM_SHADOW_CASCADES>& percents);
    void cleanup(GfxDevice& gfxDevice);

    void draw(
        VkCommandBuffer cmd,
        const BaseRenderer& baseRenderer,
        const Camera& camera,
        const glm::vec3& sunlightDirection,
        const std::vector<DrawCommand>& drawCommands,
        bool shadowsEnabled);

    const AllocatedImage& getShadowMap() { return csmShadowMap; }

    std::array<float, NUM_SHADOW_CASCADES> cascadeFarPlaneZs{};
    std::array<glm::mat4, NUM_SHADOW_CASCADES> csmLightSpaceTMs{};

    // how cascades are distributed
    std::array<float, NUM_SHADOW_CASCADES> percents;

private:
    void initCSMData(GfxDevice& device);

    AllocatedImage csmShadowMap;
    float shadowMapTextureSize{4096.f};
    std::array<Camera, NUM_SHADOW_CASCADES> cascadeCameras;
    std::array<VkImageView, NUM_SHADOW_CASCADES> csmShadowMapViews;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    struct PushConstants {
        glm::mat4 mvp;
        VkDeviceAddress vertexBuffer;
    };
};
