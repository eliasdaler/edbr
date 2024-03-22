#pragma once

#include <span>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <Graphics/BaseRenderer.h>
#include <Graphics/DrawCommand.h>
#include <Graphics/GPUMesh.h>
#include <Graphics/GfxDevice.h>
#include <Graphics/Light.h>
#include <Graphics/SpriteRenderer.h>

#include <Graphics/Pipelines/CSMPipeline.h>
#include <Graphics/Pipelines/DepthResolvePipeline.h>
#include <Graphics/Pipelines/MeshPipeline.h>
#include <Graphics/Pipelines/PostFXPipeline.h>
#include <Graphics/Pipelines/SkinningPipeline.h>
#include <Graphics/Pipelines/SkyboxPipeline.h>

#include <Graphics/Vulkan/NBuffer.h>

struct SDL_Window;

class Camera;

class GameRenderer {
public:
    struct SceneData {
        const Camera& camera;
        glm::vec4 ambientColor;
        float ambientIntensity;
        glm::vec4 fogColor;
        float fogDensity;
    };

public:
    GameRenderer(GfxDevice& gfxDevice);
    void init();

    void draw(const Camera& camera, const SceneData& sceneData);
    void cleanup();

    void updateDevTools(float dt);

    void setSkyboxImage(ImageId skyboxImageId);
    void beginDrawing();
    void endDrawing();

    void addLight(const Light& light, const Transform& transform);
    void drawMesh(MeshId id, const glm::mat4& transform, bool castShadow);
    void drawSkinnedMesh(
        std::span<const MeshId> meshes,
        std::span<const SkinnedMesh> skinnedMeshes,
        const glm::mat4& transform,
        std::span<const glm::mat4> jointMatrices);

    SkinnedMesh createSkinnedMesh(MeshId id) const;

    GfxDevice& getGfxDevice() { return gfxDevice; }

    BaseRenderer& getBaseRenderer() { return baseRenderer; }
    SpriteRenderer& getSpriteRenderer() { return spriteRenderer; }

private:
    void createDrawImage(VkExtent2D extent, bool firstCreate);
    void initSceneData();
    void updateSceneDataDescriptorSet();

    bool isMultisamplingEnabled() const;
    void onMultisamplingStateUpdate();

    void draw(VkCommandBuffer cmd, const Camera& camera, const SceneData& sceneData);

    void sortDrawList();

    GfxDevice& gfxDevice;
    BaseRenderer baseRenderer;

    SkinningPipeline skinningPipeline;
    CSMPipeline csmPipeline;
    MeshPipeline meshPipeline;
    SkyboxPipeline skyboxPipeline;
    DepthResolvePipeline depthResolvePipeline;
    PostFXPipeline postFXPipeline;

    std::vector<DrawCommand> drawCommands;
    std::vector<std::size_t> sortedDrawCommands;

    AllocatedImage drawImage;
    AllocatedImage resolveImage;

    AllocatedImage depthImage;
    AllocatedImage resolveDepthImage;

    AllocatedImage postFXDrawImage;

    bool shadowsEnabled{true};
    VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};

    // keep in sync with scene_data.glsl
    struct GPUSceneData {
        // camera
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewProj;
        glm::vec4 cameraPos;

        // ambient
        glm::vec3 ambientColor;
        float ambientIntensity;

        // fog
        glm::vec3 fogColor;
        float fogDensity;

        // CSM data
        glm::vec4 cascadeFarPlaneZs;
        std::array<glm::mat4, CSMPipeline::NUM_SHADOW_CASCADES> csmLightSpaceTMs;
        std::uint32_t csmShadowMapId;

        VkDeviceAddress lightsBuffer;
        std::uint32_t numLights;
        std::int32_t sunlightIndex;

        VkDeviceAddress materialsBuffer;
    };
    NBuffer sceneDataBuffer;
    VkDescriptorSetLayout sceneDataDescriptorLayout;
    VkDescriptorSet sceneDataDescriptorSet;

    NBuffer lightDataBuffer;
    static const int MAX_LIGHTS = 100;
    std::vector<GPULightData> lightDataCPU;
    const float pointLightMaxRange{25.f};
    const float spotLightMaxRange{64.f};
    std::int32_t sunlightIndex{-1}; // index of sun light inside the light data buffer

    SpriteRenderer spriteRenderer;
};
