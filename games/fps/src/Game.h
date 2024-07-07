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

    ImageId getMainDrawImageId() const override { return renderer.getDrawImageId(); }

private:
    void drawGameWorld(VkCommandBuffer cmd);
    void drawUI();

    glm::ivec2 gameWindowPos;
    glm::ivec2 gameWindowSize;

    // dev
    bool gameDrawnInWindow{false};
    bool drawImGui{false};

    MeshCache meshCache;
    MaterialCache materialCache;

    std::vector<MeshId> catMeshes;
    std::vector<MaterialId> catMaterials;

    GameRenderer renderer;

    Font defaultFont;

    SpriteRenderer spriteRenderer;

    Camera camera;
};
