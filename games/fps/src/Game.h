#pragma once

#include <edbr/Application.h>
#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/SpriteRenderer.h>

class Game : public Application {
public:
    void customInit() override;
    void loadAppSettings() override;
    void customCleanup() override;

    void customUpdate(float dt) override;
    void customDraw() override;

    ImageId getMainDrawImageId() const override { return gameScreenDrawImageId; }

private:
    void drawGameWorld();
    void drawUI();

    glm::ivec2 gameWindowPos;
    glm::ivec2 gameWindowSize;

    VkFormat drawImageFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    ImageId gameScreenDrawImageId{NULL_IMAGE_ID}; // image to which game pixels are drawn to

    // dev
    bool gameDrawnInWindow{false};
    bool drawImGui{false};

    Font defaultFont;

    SpriteRenderer spriteRenderer;
};
