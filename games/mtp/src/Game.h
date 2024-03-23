#pragma once

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

#include "DevTools/CustomEntityTreeView.h"
#include <edbr/DevTools/EntityInfoDisplayer.h>

#include "EntityInitializer.h"
#include "GameUI.h"
#include "Level.h"

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

    void onTagComponentDestroy(entt::registry& registry, entt::entity entity);
    void onSkeletonComponentDestroy(entt::registry& registry, entt::entity entity);

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

    bool orbitCameraAroundSelectedEntity{false};
    float orbitDistance{8.5f};

    // only display update FPS every 1 seconds, otherwise it's too noisy
    float displayedFPS{0.f};
    float displayFPSDelay{1.f};
};
