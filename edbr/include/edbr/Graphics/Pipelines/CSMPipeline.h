#pragma once

#include <array>
#include <vector>

#include <vulkan/vulkan.h>

#include <edbr/Graphics/Camera.h>

#include <edbr/Graphics/IdTypes.h>

class GfxDevice;
class MeshCache;
struct MeshDrawCommand;
struct GPUBuffer;

class CSMPipeline {
public:
    static const int NUM_SHADOW_CASCADES = 3;

public:
    void init(GfxDevice& gfxDevice, const std::array<float, NUM_SHADOW_CASCADES>& percents);
    void cleanup(GfxDevice& gfxDevice);

    void draw(
        VkCommandBuffer cmd,
        const GfxDevice& gfxDevice,
        const MeshCache& meshCache,
        const Camera& camera,
        const glm::vec3& sunlightDirection,
        const GPUBuffer& materialsBuffer,
        const std::vector<MeshDrawCommand>& meshDrawCommands,
        bool shadowsEnabled);

    ImageId getShadowMap() { return csmShadowMapID; }

    std::array<float, NUM_SHADOW_CASCADES> cascadeFarPlaneZs{};
    std::array<glm::mat4, NUM_SHADOW_CASCADES> csmLightSpaceTMs{};

    // how cascades are distributed
    std::array<float, NUM_SHADOW_CASCADES> percents;

private:
    void initCSMData(GfxDevice& device);

    ImageId csmShadowMapID{NULL_IMAGE_ID};
    float shadowMapTextureSize{4096.f};
    std::array<Camera, NUM_SHADOW_CASCADES> cascadeCameras;
    std::array<VkImageView, NUM_SHADOW_CASCADES> csmShadowMapViews;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    struct PushConstants {
        glm::mat4 mvp;
        VkDeviceAddress vertexBuffer;
        VkDeviceAddress materialsBuffer;
        std::uint32_t materialId;
        std::uint32_t padding;
    };
};
