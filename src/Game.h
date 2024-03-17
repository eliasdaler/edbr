#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <Graphics/Camera.h>

#include "FreeCameraController.h"

#include <Graphics/Font.h>
#include <Graphics/GameRenderer.h>
#include <Graphics/SkeletalAnimation.h>
#include <Graphics/SkeletonAnimator.h>
#include <Graphics/Sprite.h>

struct SDL_Window;

struct Scene;
struct SceneNode;

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

        std::vector<MeshId> meshes;
        std::vector<Transform> meshTransforms;

        // skeleton
        Skeleton skeleton;
        bool hasSkeleton{false};
        std::vector<SkinnedMesh> skinnedMeshes;

        // animation
        SkeletonAnimator skeletonAnimator;
        std::unordered_map<std::string, SkeletalAnimation> animations;

        // light
        Light light;
        bool isLight{false};
    };

public:
    Game();
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
    void destroyEntity(const Entity& e);
    Entity& findEntityByName(std::string_view name) const;

    void updateEntityTransforms();
    void updateEntityTransforms(Entity& e, const glm::mat4& parentWorldTransform);

    void generateDrawList();
    void sortDrawList();

    GfxDevice gfxDevice;
    GameRenderer renderer;

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

    glm::vec4 ambientColor;
    float ambientIntensity;
    glm::vec4 fogColor;
    float fogDensity;

    Font defaultFont;
    SpriteDrawingPipeline uiDrawingPipeline;

    std::vector<std::string> strings;

    Sprite interactipTipSprite;
    Sprite talkTipSprite;
    Sprite kaeruSprite;
    Sprite testSprite;

    Transform kaeruTransform;

    glm::vec2 rectPos{64.f, 64.f};
    glm::vec2 rectSize{32.f, 32.f};
    glm::vec2 rectPivot{0.5f, 0.5f};
    glm::vec2 rectScale{2.f};
    float rectRotation{0.f};
    float borderWidth{4.f};
    bool insetBorder{true};
};
