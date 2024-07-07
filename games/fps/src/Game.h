#pragma once

#include <edbr/Application.h>
#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/SpriteRenderer.h>

#include <edbr/Graphics/GameRenderer.h>
#include <edbr/Graphics/MaterialCache.h>
#include <edbr/Graphics/MeshCache.h>

class Game : public Application {
public:
    Game();

    void customInit() override;
    void loadAppSettings() override;
    void customCleanup() override;

    void customUpdate(float dt) override;
    void customDraw() override;

    ImageId getMainDrawImageId() const override { return drawImageId; }

private:
    void drawGameWorld(VkCommandBuffer cmd);
    void drawUI();

    glm::ivec2 gameWindowPos;
    glm::ivec2 gameWindowSize;

    VkFormat drawImageFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    ImageId drawImageId{NULL_IMAGE_ID}; // image to which game pixels are drawn to

    // dev
    bool gameDrawnInWindow{false};
    bool drawImGui{false};

    MeshCache meshCache;
    MaterialCache materialCache;

    std::vector<MeshId> catMeshes;
    std::vector<MaterialId> catMaterials;

    GameRenderer gameRenderer;

    Font defaultFont;

    SpriteRenderer spriteRenderer;

    Camera camera;
};
