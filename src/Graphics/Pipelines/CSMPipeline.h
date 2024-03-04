#pragma once

#include <vector>

#include <Graphics/Camera.h>

#include <Graphics/Vulkan/Types.h>

class Renderer;
struct DrawCommand;

class CSMPipeline {
public:
    static const int NUM_SHADOW_CASCADES = 3;

public:
    CSMPipeline(Renderer& renderer, const std::array<float, NUM_SHADOW_CASCADES>& percents);
    void cleanup(VkDevice device);

    void draw(
        VkCommandBuffer cmd,
        const Camera& camera,
        const glm::vec3& sunlightDirection,
        const std::vector<DrawCommand>& drawCommands);

    const AllocatedImage& getShadowMap() { return csmShadowMap; }

    std::array<float, NUM_SHADOW_CASCADES> cascadeFarPlaneZs{};
    std::array<glm::mat4, NUM_SHADOW_CASCADES> csmLightSpaceTMs{};

    // how cascades are distributed
    std::array<float, NUM_SHADOW_CASCADES> percents;

private:
    void initCSMData();

    Renderer& renderer;

    AllocatedImage csmShadowMap;
    float shadowMapTextureSize{4096.f};
    std::array<Camera, NUM_SHADOW_CASCADES> cascadeCameras;
    std::array<VkImageView, NUM_SHADOW_CASCADES> csmShadowMapViews;

    VkPipelineLayout meshDepthOnlyPipelineLayout;
    VkPipeline meshDepthOnlyPipeline;
};
