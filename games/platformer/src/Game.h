#pragma once

#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>

#include <entt/entity/registry.hpp>

#include <edbr/Application.h>
#include <edbr/ECS/EntityFactory.h>
#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/IdTypes.h>
#include <edbr/Graphics/SpriteAnimationData.h>
#include <edbr/Graphics/SpriteRenderer.h>
#include <edbr/TileMap/TileMapRenderer.h>

#include <edbr/DevTools/EntityInfoDisplayer.h>
#include <edbr/DevTools/EntityTreeView.h>

#include "Level.h"

class ComponentFactory;

class Game : public Application {
public:
    Game();

    void customInit() override;
    void loadAppSettings() override;
    void customCleanup() override;

    void customUpdate(float dt) override;
    void customDraw() override;

private:
    void loadAnimations(const std::filesystem::path& animationsDir);

    void initEntityFactory();
    void loadPrefabs(const std::filesystem::path& prefabsDir);
    void registerComponents(ComponentFactory& componentFactory);
    void registerComponentDisplayers();
    void entityPostInit(entt::handle e);

    void enterLevel();
    void spawnLevelEntities();
    entt::handle createEntityFromPrefab(
        const std::string& prefabName,
        const glm::vec2& spawnPos = {});

    void handleInput(float dt);
    void handlePlayerInput(float dt);
    void handleFreeCameraInput(float dt);

    void drawWorld();
    void drawGameObjects();
    void drawUI();

    void createDrawImage(const glm::ivec2& drawImageSize);

    void devToolsHandleInput(float dt);
    void devToolsUpdate(float dt);
    void devToolsDrawInWorldUI();

    glm::vec2 gameWindowPos;
    glm::vec2 gameWindowSize;

    VkFormat drawImageFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    ImageId drawImageId{NULL_IMAGE_ID};

    SpriteRenderer spriteRenderer;
    SpriteRenderer uiRenderer;
    TileMapRenderer tileMapRenderer;

    glm::vec2 cameraPos;

    Font defaultFont;
    Level level;

    std::unordered_map<std::string, SpriteAnimationData> animationsData;
    EntityFactory entityFactory;
    entt::registry registry;

    EntityTreeView entityTreeView;
    EntityInfoDisplayer entityInfoDisplayer;
    bool gameDrawnInWindow{false};
    bool drawImGui{true};
    bool freeCamera{false};
    bool drawCollisionShapes{false};
};
