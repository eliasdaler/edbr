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

#include <edbr/GameUI/Cursor.h>
#include <edbr/GameUI/DialogueBox.h>

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

    void changeLevel(const std::string& levelName, const std::string& spawnName);
    void doLevelChange();
    void enterLevel();
    void exitLevel();

    void spawnLevelEntities();
    void destroyNonPersistentEntities();

    entt::handle createEntityFromPrefab(
        const std::string& prefabName,
        const nlohmann::json& overrideData = {});
    void destroyEntity(entt::handle e, bool removeFromRegistry);

    void handleInput(float dt);
    void handlePlayerInput(float dt);
    void handleFreeCameraInput(float dt);

    glm::vec2 getMouseGameScreenPos() const;
    glm::vec2 getMouseWorldPos() const;

    void drawWorld();
    void drawGameObjects();
    void drawUI();

    void createDrawImage(const glm::ivec2& drawImageSize);

    void devToolsHandleInput(float dt);
    void devToolsUpdate(float dt);
    void devToolsDrawInWorldUI();

    // data
    glm::ivec2 gameWindowPos;
    glm::ivec2 gameWindowSize;

    VkFormat drawImageFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    ImageId drawImageId{NULL_IMAGE_ID};

    SpriteRenderer spriteRenderer;
    SpriteRenderer uiRenderer;
    TileMapRenderer tileMapRenderer;

    glm::vec2 cameraPos;
    entt::handle interactEntity;

    Level level;
    std::string newLevelToLoad;
    std::string newLevelSpawnName;

    std::unordered_map<std::string, SpriteAnimationData> animationsData;
    EntityFactory entityFactory;
    entt::registry registry;

    // ui
    DialogueBox dialogueBox;
    Font defaultFont;
    Cursor cursor;

    // dev
    Font devToolsFont;
    EntityTreeView entityTreeView;
    EntityInfoDisplayer entityInfoDisplayer;
    bool gameDrawnInWindow{false};
    bool freeCamera{false};
    bool drawImGui{true};
    bool drawCollisionShapes{false};
    bool drawEntityTags{false};
};
