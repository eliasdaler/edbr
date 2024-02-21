#pragma once

#include <memory>
#include <vector>

#include <Graphics/Camera.h>

#include "FreeCameraController.h"
#include "Renderer.h"

class SDL_Window;

class Game {
public:
    static const std::size_t NULL_ENTITY_ID = std::numeric_limits<std::size_t>::max();

    using EntityId = std::size_t;

    struct Entity {
        EntityId id{NULL_ENTITY_ID};
        std::string tag;

        // transform
        Transform transform; // local (relative to parent)
        glm::mat4 worldTransform{1.f};

        // hierarchy
        EntityId parentId{NULL_ENTITY_ID};
        std::vector<EntityId> children;

        // mesh (only one mesh per entity supported for now)
        std::vector<MeshId> meshes;

        // skeleton
        // Skeleton skeleton;
        // bool hasSkeleton{false};

        // animation
        // SkeletonAnimator skeletonAnimator;
        // std::unordered_map<std::string, SkeletalAnimation> animations;

        /* void uploadJointMatricesToGPU(
            const wgpu::Queue& queue,
            const std::vector<glm::mat4>& jointMatrices) const; */
    };

public:
    void init();
    void run();
    void cleanup();

private:
    void handleInput(float dt);
    void update(float dt);
    void updateDevTools(float dt);

    void createEntitiesFromScene(const Scene& scene);
    EntityId createEntitiesFromNode(
        const Scene& scene,
        const SceneNode& node,
        EntityId parentId = NULL_ENTITY_ID);

    std::vector<std::unique_ptr<Entity>> entities;
    Entity& makeNewEntity();
    Entity& findEntityByName(std::string_view name) const;

    void updateEntityTransforms();
    void updateEntityTransforms(Entity& e, const glm::mat4& parentWorldTransform);

    void generateDrawList();
    void sortDrawList();

    Renderer renderer;

    SDL_Window* window;

    bool isRunning{false};
    bool vSync{true};
    bool frameLimit{true};
    float frameTime{0.f};
    float avgFPS{0.f};

    // only display update FPS every 1 seconds, otherwise it's too noisy
    float displayedFPS{0.f};
    float displayFPSDelay{1.f};

    Camera camera;
    FreeCameraController cameraController;
};
