#pragma once

#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>

#include <edbr/Application.h>
#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/IdTypes.h>
#include <edbr/Graphics/Sprite.h>
#include <edbr/Graphics/SpriteAnimationData.h>
#include <edbr/Graphics/SpriteAnimator.h>
#include <edbr/Graphics/SpriteRenderer.h>
#include <edbr/TileMap/TileMapRenderer.h>

#include "Level.h"

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

    void handleInput(float dt);
    void handlePlayerInput(float dt);
    void handleFreeCameraInput(float dt);

    void drawWorld();
    void drawGameObjects();
    void drawUI();

    void createDrawImage(const glm::ivec2& drawImageSize);

    glm::vec2 gameWindowPos;
    glm::vec2 gameWindowSize;

    VkFormat drawImageFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    ImageId drawImageId{NULL_IMAGE_ID};

    SpriteRenderer spriteRenderer;
    SpriteRenderer uiRenderer;
    TileMapRenderer tileMapRenderer;

    glm::vec2 cameraPos;
    glm::vec2 playerPos;

    SpriteAnimator playerSpriteAnimator;

    Sprite playerSprite;
    Font defaultFont;
    Level level;

    bool gameDrawnInWindow{false};
    bool drawImGui{true};
    bool freeCamera{false};

    std::unordered_map<std::string, SpriteAnimationData> animationsData;
};
