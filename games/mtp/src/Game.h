#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <entt/entity/registry.hpp>

#include <edbr/Application.h>
#include <edbr/ECS/EntityFactory.h>

#include <edbr/Graphics/BaseRenderer.h>
#include <edbr/Graphics/Camera.h>
#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/GameRenderer.h>
#include <edbr/Graphics/Scene.h>
#include <edbr/Graphics/SpriteRenderer.h>

#include <edbr/FreeCameraController.h>
#include <edbr/SceneCache.h>

#include <edbr/DevTools/EntityInfoDisplayer.h>
#include <edbr/DevTools/Im3dState.h>
#include <edbr/DevTools/ResourcesInspector.h>

#include "DevTools/CustomEntityTreeView.h"

#include "EntityInitializer.h"
#include "GameSceneLoader.h"
#include "GameUI.h"
#include "Level.h"
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

    util::SceneLoadContext createSceneLoadContext();

private:
    BaseRenderer baseRenderer;
    GameRenderer renderer;
    SpriteRenderer spriteRenderer;

    Camera camera;
    FreeCameraController cameraController;

    GameUI ui;
    Level level;
    std::filesystem::path skyboxDir;

    SceneCache sceneCache;

    entt::registry registry;
    std::unordered_map<std::string, entt::entity> taggedEntities;
    EntityFactory entityFactory;
    EntityInitializer entityInitializer;

    CustomEntityTreeView entityTreeView;
    EntityInfoDisplayer entityInfoDisplayer;
    ResourcesInspector resourcesInspector;

    bool orbitCameraAroundSelectedEntity{false};

    // only display update FPS every 1 seconds, otherwise it's too noisy
    float displayedFPS{0.f};
    float displayFPSDelay{1.f};

    bool gameDrawnInWindow{false};
    const char* gameWindowLabel{"Game##window"};
    bool drawEntityTags{false};
    bool drawEntityHeading{false};
    bool drawImGui{true};
    bool drawGizmos{false};

    glm::ivec2 gameWindowPos;
    glm::ivec2 gameWindowSize;

    Im3dState im3d;

    std::unique_ptr<PhysicsSystem> physicsSystem;

    glm::vec3 prevPlayerPos;

    glm::vec3 followCameraVelocity;
    glm::vec3 cameraCurrentTrackPointPos;

    bool smoothCamera{true};
    float cameraDelay{0.3f};
    float orbitDistance{6.5f};
    float cameraMaxSpeed{6.5f};
    float cameraYOffset{1.5f};
    float cameraZOffset{0.25f};
    float testParam{0.4f};

    float timeWalkingOrRunning{0.f};
    bool walkingOrRunning{false};
    bool wasWalkingOrRunning{false};
    bool running{false};
    bool wasRunning{false};

    float prevOffsetZ{cameraZOffset};
    float interpolationOffsetZStart{cameraZOffset};
    float movingToMaxOffsetTime{0.f};
    float cameraMaxOffsetTime{2.f}; // when time reaches maximum offset
    float maxCameraOffsetFactorRun{10.0f};
    float maxCameraOffsetFactorWalk{1.f}; // TODO: explore if it's worth it
    bool drawCameraTrackPoint{false};
    // camera moves to run offset when the speed is big, not just when the character runs
    bool runningCamera{false};
    bool wasRunningCamera{false};

    float cameraNear{1.f};
    float cameraFar{64.f};
    float cameraFovX{glm::radians(45.f)};
};
