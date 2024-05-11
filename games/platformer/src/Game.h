#pragma once

#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>

#include <entt/entity/registry.hpp>

#include <edbr/Application.h>
#include <edbr/ECS/EntityFactory.h>
#include <edbr/Graphics/Camera.h>
#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/IdTypes.h>
#include <edbr/Graphics/SpriteAnimationData.h>
#include <edbr/Graphics/SpriteRenderer.h>
#include <edbr/Text/TextToken.h>
#include <edbr/TileMap/TileMapRenderer.h>

#include <edbr/DevTools/EntityInfoDisplayer.h>
#include <edbr/DevTools/EntityTreeView.h>
#include <edbr/DevTools/UIInspector.h>

#include "GameUI.h"
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

public:
    // high level functions for scripts
    [[nodiscard]] ActionList say(const LocalizedStringTag& text, entt::handle speaker = {});
    [[nodiscard]] ActionList say(const dialogue::TextToken& textToken, entt::handle speaker = {});
    [[nodiscard]] ActionList say(
        std::span<const dialogue::TextToken> textTokens,
        entt::handle speaker = {});

private:
    void loadAnimations(const std::filesystem::path& animationsDir);

    void initEntityFactory();
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
    void destroyEntity(entt::handle e);

    void handleInput(float dt);
    void handlePlayerInput(const ActionMapping& am, float dt);

    void handleInteraction();

    glm::ivec2 getGameScreenSize() const;
    glm::vec2 getMouseGameScreenPos() const;
    glm::vec2 getMouseWorldPos() const;

    void drawWorld();
    void drawGameObjects();
    void drawUI();

    void devToolsHandleInput(float dt);
    void handleFreeCameraInput(float dt);
    void devToolsUpdate(float dt);
    void devToolsDrawInWorldUI();

    // data
    glm::ivec2 gameWindowPos;
    glm::ivec2 gameWindowSize;

    VkFormat drawImageFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    ImageId drawImageId{NULL_IMAGE_ID};

    std::unordered_map<std::string, SpriteAnimationData> animationsData;
    EntityFactory entityFactory;
    entt::registry registry;

    SpriteRenderer spriteRenderer;
    SpriteRenderer uiRenderer;
    TileMapRenderer tileMapRenderer;

    Level level;
    std::string newLevelToLoad;
    std::string newLevelSpawnName;

    Camera gameCamera;
    entt::handle interactEntity;
    bool playerInputEnabled{true};

    // ui
    GameUI ui;

    // dev
    Font devToolsFont;
    EntityTreeView entityTreeView;
    EntityInfoDisplayer entityInfoDisplayer;
    UIInspector uiInspector;
    bool gameDrawnInWindow{false};
    bool freeCamera{false};
    bool drawImGui{true};
    bool drawCollisionShapes{false};
    bool drawEntityTags{false};
    // only display update FPS every 1 seconds, otherwise it's too noisy
    float displayedFPS{0.f};
    float displayFPSDelay{1.f};
};
