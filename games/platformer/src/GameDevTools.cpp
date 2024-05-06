#include "Game.h"

#include "Components.h"
#include "EntityUtil.h"

#include <edbr/DevTools/ImGuiPropertyTable.h>
#include <edbr/Graphics/CoordUtil.h>
#include <edbr/Util/ImGuiUtil.h>
#include <imgui.h>

namespace
{
math::FloatRect getSelectedEntityRect(entt::const_handle e)
{
    if (e.all_of<SpriteComponent>()) {
        return entityutil::getSpriteWorldRect(e);
    }
    return {};
}
}

void Game::devToolsHandleInput(float dt)
{
    if (inputManager.getKeyboard().wasJustPressed(SDL_SCANCODE_C)) {
        freeCamera = !freeCamera;
    }
    if (inputManager.getKeyboard().wasJustPressed(SDL_SCANCODE_B)) {
        drawCollisionShapes = !drawCollisionShapes;
    }
}

void Game::devToolsUpdate(float dt)
{
    if (ImGui::Begin("Hello, world")) {
        using namespace devtools;
        BeginPropertyTable();
        const auto& mousePos = inputManager.getMouse().getPosition();
        const auto& gameScreenPos = edbr::util::
            getGameWindowScreenCoord(mousePos, gameWindowPos, gameWindowSize, params.renderSize);
        DisplayProperty("Mouse pos", mousePos);
        DisplayProperty("Game screen pos", gameScreenPos);
        const auto worldPos = static_cast<glm::vec2>(gameScreenPos) + cameraPos;
        DisplayProperty("Mouse wolrd pos", worldPos);
        DisplayProperty("Tile index", TileMap::GetTileIndex(worldPos));
        EndPropertyTable();
        ImGui::End();
    }

    if (ImGui::Begin("Entities")) {
        entityTreeView.update(registry, dt);
        ImGui::End();
    }

    if (entityTreeView.hasSelectedEntity()) {
        if (ImGui::Begin("Selected entity")) {
            entityInfoDisplayer.displayEntityInfo(entityTreeView.getSelectedEntity(), dt);
            ImGui::End();
        }
    }
}

void Game::devToolsDrawInWorldUI()
{
    if (entityTreeView.hasSelectedEntity()) {
        const auto selectionRect = getSelectedEntityRect(entityTreeView.getSelectedEntity());

        auto bbColor = LinearColor{0.f, 0.f, 1.f};
        { // simple way to "flash" the selected entity BB
            static int frameNum = 0;
            ++frameNum;
            float v = std::abs(std::sin((float)frameNum / 60.f));
            bbColor.r *= v;
            bbColor.g *= v;
            bbColor.b *= v;
            bbColor.a = 0.5f;
        }
        spriteRenderer.drawInsetRect(selectionRect, bbColor);
    }

    if (drawCollisionShapes) {
        tileMapRenderer.drawTileMapLayer(
            gfxDevice,
            spriteRenderer,
            level.getTileMap(),
            level.getTileMap().getLayer(TileMap::CollisionLayerName));
        for (const auto&& [e, cc] : registry.view<CollisionComponent>().each()) {
            auto bb = entityutil::getAABB({registry, e});
            spriteRenderer.drawFilledRect(bb, LinearColor{1.f, 0.f, 0.f, 0.5f});
        }
    }
}
