#pragma once

#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>

#include <edbr/Application.h>
#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/IdTypes.h>
#include <edbr/Graphics/Sprite.h>
#include <edbr/Graphics/SpriteRenderer.h>

class Game : public Application {
public:
    Game();

    void customInit() override;
    void loadAppSettings() override;
    void customCleanup() override;

    void customUpdate(float dt) override;
    void customDraw() override;

private:
    void createDrawImage(const glm::ivec2& drawImageSize);

    glm::vec2 gameWindowPos;
    glm::vec2 gameWindowSize;

    VkFormat drawImageFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    ImageId drawImageId{NULL_IMAGE_ID};
    SpriteRenderer spriteRenderer;

    Sprite playerSprite;
    Font defaultFont;

    bool gameDrawnInWindow{false};
    bool drawImGui{true};
};
