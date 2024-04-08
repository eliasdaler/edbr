#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <entt/entity/registry.hpp>

#include <edbr/Application.h>
#include <edbr/ECS/EntityFactory.h>

#include <edbr/Graphics/Camera.h>
#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/GameRenderer.h>
#include <edbr/Graphics/MaterialCache.h>
#include <edbr/Graphics/MeshCache.h>
#include <edbr/Graphics/Scene.h>
#include <edbr/Graphics/SpriteRenderer.h>

#include <edbr/FreeCameraController.h>
#include <edbr/Graphics/SkeletalAnimationCache.h>
#include <edbr/SceneCache.h>

#include <edbr/DevTools/EntityInfoDisplayer.h>
#include <edbr/DevTools/Im3dState.h>
#include <edbr/DevTools/ResourcesInspector.h>

#include "DevTools/CustomEntityTreeView.h"

#include "EntityCreator.h"
#include "GameUI.h"
#include "Level.h"

#include "AnimationSoundSystem.h"
#include "PhysicsSystem.h"

class ComponentFactory;

class Game : public Application {
public:
    Game();

    void customInit() override;
    void customUpdate(float dt) override;
    void customDraw() override;
    void customCleanup() override;

private:
    void initUI();
    void loadLevel(const std::filesystem::path& path);
    void loadScene(const std::filesystem::path& path, bool createEntities = false);

    void handleInput(float dt);
    void handlePlayerInput(float dt);

    void updateDevTools(float dt);

    void setEntityTag(entt::handle entity, const std::string& tag);
    entt::handle findEntityByTag(const std::string& tag);
    entt::const_handle findEntityByTag(const std::string& tag) const;

    entt::const_handle findDefaultCamera();
    void setCurrentCamera(entt::const_handle camera);

    void generateDrawList();

private:
    void initEntityFactory();
    void loadPrefabs(const std::filesystem::path& prefabsDir);
    void registerComponents(ComponentFactory& componentFactory);
    void registerComponentDisplayers();

    void updateFollowCamera(entt::const_handle followEntity, float dt);
    float calculateSmartZOffset(entt::const_handle followEntity, float dt);

    void onTagComponentDestroy(entt::registry& registry, entt::entity entity);
    void onSkeletonComponentDestroy(entt::registry& registry, entt::entity entity);

    void entityPostInit(entt::handle e);

private:
    MeshCache meshCache;
    MaterialCache materialCache;
    GameRenderer renderer;
    SpriteRenderer spriteRenderer;

    SkeletalAnimationCache animationCache;
    SceneCache sceneCache;

    entt::registry registry;
    std::unordered_map<std::string, entt::entity> taggedEntities;
    EntityFactory entityFactory;

    EntityCreator entityCreator;

    std::unique_ptr<PhysicsSystem> physicsSystem;
    AnimationSoundSystem animationSoundSystem;

    entt::handle interactEntity;

    Level level;
    std::filesystem::path skyboxDir;

    Camera camera;
    FreeCameraController cameraController;
    float cameraNear{1.f};
    float cameraFar{200.f};
    float cameraFovX{glm::radians(45.f)};

    bool orbitCameraAroundSelectedEntity{false};
    bool smoothCamera{true};
    bool smoothSmartCamera{true};
    float cameraDelay{0.3f};
    float orbitDistance{6.5f};
    float cameraMaxSpeed{6.5f};
    float cameraYOffset{1.5f};
    float cameraZOffset{0.25f};
    float testParam{0.4f};

    glm::vec3 followCameraVelocity;
    glm::vec3 cameraCurrentTrackPointPos;
    float prevOffsetZ{cameraZOffset};
    float cameraMaxOffsetTime{2.f}; // when time reaches maximum offset
    float maxCameraOffsetFactorRun{7.5f};
    bool drawCameraTrackPoint{false};

    float prevDesiredOffset{cameraZOffset};
    float desiredOffsetChangeTimer{0.25f};
    float desiredOffsetChangeDelayRun{1.25f};
    float desiredOffsetChangeDelayRecenter{1.f};

    // position and size of where the game is rendered in the window
    glm::ivec2 gameWindowPos;
    glm::ivec2 gameWindowSize;

    GameUI ui;

    // DEV
    CustomEntityTreeView entityTreeView;
    EntityInfoDisplayer entityInfoDisplayer;
    ResourcesInspector resourcesInspector;

    // only display update FPS every 1 seconds, otherwise it's too noisy
    float displayedFPS{0.f};
    float displayFPSDelay{1.f};

    bool gameDrawnInWindow{false};
    const char* gameWindowLabel{"Game##window"};
    bool drawEntityTags{false};
    bool drawEntityHeading{false};
    bool drawImGui{true};
    bool drawGizmos{false};

    Im3dState im3d;
};
