#pragma once

#include <array>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include <edbr/Graphics/Camera.h>
#include <edbr/Graphics/Light.h>
#include <edbr/Graphics/NBuffer.h>
#include <edbr/Graphics/Vulkan/GPUBuffer.h>

#include <edbr/Graphics/IdTypes.h>

class GfxDevice;
class MeshCache;
struct MeshDrawCommand;
struct Light;

class PointLightShadowMapPipeline {
public:
    void init(GfxDevice& gfxDevice, float pointLightMaxRange);
    void cleanup(GfxDevice& gfxDevice);

    void beginFrame(
        VkCommandBuffer cmd,
        const GfxDevice& gfxDevice,
        const std::vector<GPULightData>& lightData,
        const std::vector<std::size_t> pointLightIndices);
    void draw(
        VkCommandBuffer cmd,
        const GfxDevice& gfxDevice,
        const MeshCache& meshCache,
        const Camera& camera,
        const std::uint32_t lightIndex,
        const glm::vec3& lightPos,
        const GPUBuffer& sceneDataBuffer,
        const std::vector<MeshDrawCommand>& meshDrawCommands,
        bool shadowsEnabled);
    void endFrame(VkCommandBuffer cmd, const GfxDevice& gfxDevice);

    const std::unordered_map<std::uint32_t, ImageId>& getLightToShadowMapId() const
    {
        return lightToShadowMapId;
    };

private:
    void initCSMData(GfxDevice& device);

    float pointLightMaxRange{0.f}; // set in init function
    float shadowMapTextureSize{1024.f};

    static constexpr int MAX_POINT_LIGHTS = 16;
    std::array<Camera, MAX_POINT_LIGHTS * 6> pointLightShadowMapCameras;
    std::array<glm::mat4, MAX_POINT_LIGHTS * 6> pointLightShadowMapVPs;

    std::array<ImageId, MAX_POINT_LIGHTS> shadowMaps;
    std::vector<VkImageView> shadowMapImageViews;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    struct PushConstants {
        glm::mat4 model;
        VkDeviceAddress vertexBuffer;
        VkDeviceAddress sceneDataBuffer;
        VkDeviceAddress vpsBuffer;
        std::uint32_t materialId;
        std::uint32_t lightIndex;
        std::uint32_t vpsBufferIndex;
        float farPlane;
    };

    std::unordered_map<std::uint32_t, ImageId> lightToShadowMapId;
    std::uint32_t currShadowMapIndex{0};

    NBuffer vpsBuffer;
};
