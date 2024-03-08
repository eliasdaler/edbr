#pragma once

#include <filesystem>
#include <memory>
#include <span>

#include <glm/vec4.hpp>

#include <Graphics/BaseRenderer.h>
#include <Graphics/DrawCommand.h>
#include <Graphics/GPUMesh.h>
#include <Graphics/Renderer.h>

#include <Graphics/Pipelines/CSMPipeline.h>
#include <Graphics/Pipelines/DepthResolvePipeline.h>
#include <Graphics/Pipelines/MeshPipeline.h>
#include <Graphics/Pipelines/PostFXPipeline.h>
#include <Graphics/Pipelines/SkinningPipeline.h>
#include <Graphics/Pipelines/SkyboxPipeline.h>

#include <Graphics/Vulkan/NBuffer.h>

struct SDL_Window;

class Camera;
struct Scene;

class GameRenderer {
public:
    struct SceneData {
        const Camera& camera;
        glm::vec4 ambientColorAndIntensity;
        glm::vec4 sunlightDirection;
        glm::vec4 sunlightColorAndIntensity;
        glm::vec4 fogColorAndDensity;
    };

public:
    GameRenderer();
    void init(SDL_Window* window, bool vSync);

    void draw(const Camera& camera, const SceneData& sceneData);
    void cleanup();

    void updateDevTools(float dt);

    Scene loadScene(const std::filesystem::path& path);

    void beginDrawing();
    void endDrawing();

    void addDrawCommand(MeshId id, const glm::mat4& transform, bool castShadow);
    void addDrawSkinnedMeshCommand(
        std::span<const MeshId> meshes,
        std::span<const SkinnedMesh> skinnedMeshes,
        const glm::mat4& transform,
        std::span<const glm::mat4> jointMatrices);

    Renderer& getRenderer() { return renderer; }

    SkinnedMesh createSkinnedMesh(MeshId id) const;

private:
    void createDrawImage(VkExtent2D extent, bool firstCreate);
    void initSceneData();
    void updateSceneDataDescriptorSet();

    bool isMultisamplingEnabled() const;
    void onMultisamplingStateUpdate();

    void draw(VkCommandBuffer cmd, const Camera& camera, const SceneData& sceneData);

    void sortDrawList();

    Renderer renderer;
    BaseRenderer baseRenderer;

    std::unique_ptr<SkinningPipeline> skinningPipeline;
    std::unique_ptr<CSMPipeline> csmPipeline;
    std::unique_ptr<MeshPipeline> meshPipeline;
    std::unique_ptr<SkyboxPipeline> skyboxPipeline;
    std::unique_ptr<DepthResolvePipeline> depthResolvePipeline;
    std::unique_ptr<PostFXPipeline> postFXPipeline;

    std::vector<DrawCommand> drawCommands;
    std::vector<std::size_t> sortedDrawCommands;

    AllocatedImage drawImage;
    AllocatedImage resolveImage;

    AllocatedImage depthImage;
    AllocatedImage resolveDepthImage;

    AllocatedImage postFXDrawImage;

    AllocatedImage skyboxImage;

    bool shadowsEnabled{true};
    VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};

    struct GPUSceneData {
        // camera
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewProj;
        glm::vec4 cameraPos;

        // ambient
        glm::vec4 ambientColorAndIntensity;

        // sun
        glm::vec4 sunlightDirection;
        glm::vec4 sunlightColorAndIntensity;

        // CSM data
        glm::vec4 cascadeFarPlaneZs;
        std::array<glm::mat4, 3> csmLightSpaceTMs;
    };
    NBuffer sceneDataBuffer;
    VkDescriptorSetLayout sceneDataDescriptorLayout;
    VkDescriptorSet sceneDataDescriptorSet;
};
