#pragma once

#include <filesystem>
#include <memory>
#include <span>

#include <glm/vec4.hpp>

#include <Graphics/DrawCommand.h>
#include <Graphics/GPUMesh.h>
#include <Graphics/Renderer.h>

#include <Graphics/Pipelines/BackgroundGradientPipeline.h>
#include <Graphics/Pipelines/CSMPipeline.h>
#include <Graphics/Pipelines/MeshPipeline.h>
#include <Graphics/Pipelines/SkinningPipeline.h>
#include <Graphics/Pipelines/SkyboxPipeline.h>

struct SDL_Window;

class Camera;
struct Scene;

struct RendererSceneData {
    const Camera& camera;
    glm::vec4 ambientColorAndIntensity;
    glm::vec4 sunlightDirection;
    glm::vec4 sunlightColorAndIntensity;
};

class GameRenderer {
public:
    void init(SDL_Window* window, bool vSync);

    void draw(const Camera& camera, const RendererSceneData& sceneData);
    void cleanup();

    void updateDevTools(float dt);

    Scene loadScene(const std::filesystem::path& path);

    void beginDrawing();
    void endDrawing();

    void addDrawCommand(MeshId id, const glm::mat4& transform);
    void addDrawSkinnedMeshCommand(
        std::span<const MeshId> meshes,
        std::span<const SkinnedMesh> skinnedMeshes,
        const glm::mat4& transform,
        std::span<const glm::mat4> jointMatrices);

    Renderer& getRenderer() { return renderer; }

private:
    void createDrawImage(VkExtent2D extent);

    void draw(VkCommandBuffer cmd, const Camera& camera, const RendererSceneData& sceneData);

    void sortDrawList();

    Renderer renderer;

    BackgroundGradientPipeline backgroundGradientPipeline;
    std::unique_ptr<SkinningPipeline> skinningPipeline;
    std::unique_ptr<CSMPipeline> csmPipeline;
    std::unique_ptr<MeshPipeline> meshPipeline;
    std::unique_ptr<SkyboxPipeline> skyboxPipeline;

    std::vector<DrawCommand> drawCommands;
    std::vector<std::size_t> sortedDrawCommands;

    AllocatedImage drawImage;
    AllocatedImage depthImage;

    AllocatedImage skyboxImage;
};
