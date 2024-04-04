#pragma once

#include <span>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <edbr/Graphics/Vulkan/GPUImage.h>

#include <edbr/Graphics/Color.h>
#include <edbr/Graphics/GPUMesh.h>
#include <edbr/Graphics/Light.h>
#include <edbr/Graphics/MeshDrawCommand.h>
#include <edbr/Graphics/NBuffer.h>

#include <edbr/Graphics/Pipelines/CSMPipeline.h>
#include <edbr/Graphics/Pipelines/DepthResolvePipeline.h>
#include <edbr/Graphics/Pipelines/MeshPipeline.h>
#include <edbr/Graphics/Pipelines/PostFXPipeline.h>
#include <edbr/Graphics/Pipelines/SkinningPipeline.h>
#include <edbr/Graphics/Pipelines/SkyboxPipeline.h>

struct SDL_Window;

class Camera;
class GfxDevice;
class MeshCache;
class MaterialCache;

class GameRenderer {
public:
    struct SceneData {
        const Camera& camera;
        LinearColor ambientColor;
        float ambientIntensity;
        LinearColor fogColor;
        float fogDensity;
    };

public:
    GameRenderer(GfxDevice& gfxDevice, MeshCache& meshCache, MaterialCache& materialCache);

    void init(const glm::ivec2& drawImageSize);
    void draw(VkCommandBuffer cmd, const Camera& camera, const SceneData& sceneData);
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

    GfxDevice& getGfxDevice() { return gfxDevice; }

    const GPUImage& getDrawImage() const;
    VkFormat getDrawImageFormat() const;
    const GPUImage& getDepthImage() const;
    VkFormat getDepthImageFormat() const;

private:
    void createDrawImage(const glm::ivec2& drawImageSize, bool firstCreate);
    void initSceneData();

    bool isMultisamplingEnabled() const;
    void onMultisamplingStateUpdate();

    void sortDrawList();

    GfxDevice& gfxDevice;
    MeshCache& meshCache;
    MaterialCache& materialCache;

    SkinningPipeline skinningPipeline;
    CSMPipeline csmPipeline;
    MeshPipeline meshPipeline;
    SkyboxPipeline skyboxPipeline;
    DepthResolvePipeline depthResolvePipeline;
    PostFXPipeline postFXPipeline;

    std::vector<MeshDrawCommand> meshDrawCommands;
    std::vector<std::size_t> sortedMeshDrawCommands;

    VkFormat drawImageFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    VkFormat depthImageFormat{VK_FORMAT_D32_SFLOAT};

    ImageId drawImageId{NULL_IMAGE_ID};
    ImageId resolveImageId{NULL_IMAGE_ID};
    ImageId depthImageId{NULL_IMAGE_ID};
    ImageId resolveDepthImageId{NULL_IMAGE_ID};
    ImageId postFXDrawImageId{NULL_IMAGE_ID};

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
        LinearColorNoAlpha ambientColor;
        float ambientIntensity;

        // fog
        LinearColorNoAlpha fogColor;
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

    NBuffer lightDataBuffer;
    static const int MAX_LIGHTS = 100;
    std::vector<GPULightData> lightDataCPU;
    const float pointLightMaxRange{25.f};
    const float spotLightMaxRange{64.f};
    std::int32_t sunlightIndex{-1}; // index of sun light inside the light data buffer
};