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

entt::handle findEntityUnderCursor(entt::registry& registry, const glm::vec2& mouseWorldPos)
{
    for (auto e : registry.view<entt::entity>()) {
        auto aabb = getSelectedEntityRect({registry, e});
        if (aabb.contains(mouseWorldPos)) {
            return {registry, e};
        }
    }
    return {};
}
}

void Game::devToolsHandleInput(float dt)
{
    const auto& kb = inputManager.getKeyboard();
    if (kb.wasJustPressed(SDL_SCANCODE_C)) {
        freeCamera = !freeCamera;
    }
    if (kb.wasJustPressed(SDL_SCANCODE_B)) {
        drawCollisionShapes = !drawCollisionShapes;
    }
    if (kb.wasJustPressed(SDL_SCANCODE_T)) {
        drawEntityTags = !drawEntityTags;
    }
    if (kb.wasJustPressed(SDL_SCANCODE_TAB)) {
        drawImGui = !drawImGui;
    }

    const auto& mouse = inputManager.getMouse();
    if (!ImGui::GetIO().WantCaptureMouse && mouse.wasJustPressed(SDL_BUTTON(1))) {
        const auto& mousePos = inputManager.getMouse().getPosition();
        const auto& gameScreenPos = edbr::util::
            getGameWindowScreenCoord(mousePos, gameWindowPos, gameWindowSize, params.renderSize);
        const auto screenRect = math::FloatRect{{}, static_cast<glm::vec2>(params.renderSize)};
        if (screenRect.contains(gameScreenPos)) {
            const auto mouseWorldPos = getMouseWorldPos();
            auto selected = findEntityUnderCursor(registry, getMouseWorldPos());
            entityTreeView.setSelectedEntity(selected);
        }
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
        const auto mouseWorldPos = getMouseWorldPos();
        DisplayProperty("Mouse wolrd pos", mouseWorldPos);
        DisplayProperty("Tile index", TileMap::GetTileIndex(mouseWorldPos));
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

    if (drawEntityTags) {
        for (const auto&& [e, tc] : registry.view<TagComponent>().each()) {
            // text is centered on top-center of entity rect
            const auto aabb = getSelectedEntityRect({registry, e});
            const auto textBB = devToolsFont.calculateTextBoundingBox(tc.tag);
            const auto textPos = (aabb.getPosition() + glm::vec2{aabb.width, 0.f}) -
                                 glm::vec2{textBB.width / 2.f, textBB.height};

            // "shadow"
            spriteRenderer.drawText(
                devToolsFont, tc.tag, textPos + glm::vec2{1.f, 1.f}, LinearColor::Black());

            spriteRenderer.drawText(devToolsFont, tc.tag, textPos, LinearColor{1.f, 1.f, 0.f});
        }
    }
}
