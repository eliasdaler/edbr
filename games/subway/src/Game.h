#pragma once

#include <edbr/Application.h>
#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/Sprite.h>
#include <edbr/Graphics/SpriteRenderer.h>
#include <edbr/TileMap/TileMap.h>
#include <edbr/TileMap/TileMapRenderer.h>

#include <edbr/Graphics/Pipelines/CRTPipeline.h>

class Game : public Application {
public:
    void customInit() override;
    void loadAppSettings() override;
    void customCleanup() override;

    void customUpdate(float dt) override;
    void customDraw() override;

    ImageId getMainDrawImageId() const override { return finalDrawImageId; }

    void onWindowResize() override;

private:
    void drawGameWorld();
    void drawGameObjects();
    void drawUI();

    glm::ivec2 gameWindowPos;
    glm::ivec2 gameWindowSize;

    VkFormat drawImageFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    ImageId gameScreenDrawImageId{NULL_IMAGE_ID}; // image to which game pixels are drawn to
    ImageId finalDrawImageId{NULL_IMAGE_ID}; // id of image which is drawn to the window

    CRTPipeline crtPipeline;
    SpriteRenderer spriteRenderer;

    Sprite playerSprite;
    glm::vec2 playerPos;

    Sprite fakeUISprite;

    // dev
    bool gameDrawnInWindow{false};
    bool drawImGui{false};

    Font defaultFont;
    TileMap tileMap;
    TileMapRenderer tileMapRenderer;
};
