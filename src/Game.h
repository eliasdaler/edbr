#pragma once

#include <string>
#include <unordered_map>

#include <entt/entt.hpp>

#include <ECS/EntityFactory.h>

#include <Graphics/Camera.h>
#include <Graphics/Font.h>
#include <Graphics/GameRenderer.h>
#include <Graphics/Scene.h>
#include <Graphics/Sprite.h>

#include "FreeCameraController.h"
#include "SceneCache.h"

#include "DevTools/EntityInfoDisplayer.h"
#include "DevTools/EntityTreeView.h"

#include "GameUI.h"
#include "Level.h"

struct SDL_Window;
class ComponentFactory;

class Game {
public:
public:
    Game();
    void init();
    void run();
    void cleanup();

private:
    void initUI();
    void loadLevel(const std::filesystem::path& path);
    void loadScene(const std::filesystem::path& path, bool createEntities = false);

    void handleInput(float dt);
    void handlePlayerInput(float dt);

    void update(float dt);
    void updateDevTools(float dt);

    void setEntityTag(entt::handle entity, const std::string& tag);
    entt::handle findEntityByTag(const std::string& tag);
    entt::const_handle findEntityByTag(const std::string& tag) const;

    entt::const_handle findDefaultCamera();
    void setCurrentCamera(entt::const_handle camera);

    void updateEntityTransforms();
    void updateEntityTransforms(entt::entity e, const glm::mat4& parentWorldTransform);

    void generateDrawList();

private:
    void initEntityFactory();
    void loadPrefabs(const std::filesystem::path& prefabsDir);
    void registerComponents(ComponentFactory& componentFactory);
    void registerComponentDisplayers();
    void postInitEntity(entt::handle e);

    void onTagComponentDestroy(entt::registry& registry, entt::entity entity);
    void onSkeletonComponentDestroy(entt::registry& registry, entt::entity entity);

private:
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

    GameUI ui;
    Level level;
    std::filesystem::path skyboxDir;

    entt::registry registry;
    std::unordered_map<std::string, entt::entity> taggedEntities;
    EntityFactory entityFactory;

    SceneCache sceneCache;

    EntityTreeView entityTreeView;
    EntityInfoDisplayer entityInfoDisplayer;

    bool orbitCameraAroundSelectedEntity{false};
    float orbitDistance{8.5f};
};
