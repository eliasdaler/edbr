#include "Game.h"

#include "Components.h"
#include "EntityUtil.h"

#include <edbr/DevTools/ImGuiPropertyTable.h>
#include <edbr/ECS/Components/CollisionComponent2D.h>
#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/ECS/Components/SpriteComponent.h>
#include <edbr/ECS/Components/TagComponent.h>
#include <edbr/Graphics/CoordUtil.h>
#include <edbr/Util/ImGuiUtil.h>
#include <edbr/Util/InputUtil.h>
#include <imgui.h>

namespace
{
math::FloatRect getSelectedEntityRect(entt::const_handle e)
{
    if (e.all_of<SpriteComponent>()) {
        return entityutil::getSpriteWorldRect(e);
    }
    if (e.all_of<CollisionComponent2D>()) {
        return entityutil::getCollisionAABB(e);
    }
    return {entityutil::getWorldPosition2D(e), {}};
}

entt::handle findEntityUnderCursor(
    entt::registry& registry,
    const glm::vec2& mouseWorldPos,
    bool includeInvisible)
{
    for (auto e : registry.view<entt::entity>()) {
        if (!includeInvisible && !registry.all_of<SpriteComponent>(e)) {
            continue;
        }

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
        playerInputEnabled = !freeCamera;
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
        const auto& gameScreenPos = getMouseGameScreenPos();
        const auto screenRect = math::FloatRect{{}, static_cast<glm::vec2>(params.renderSize)};
        if (screenRect.contains(gameScreenPos)) {
            const auto mouseWorldPos = getMouseWorldPos();
            bool includeInvisible = drawCollisionShapes;
            const auto selected =
                findEntityUnderCursor(registry, getMouseWorldPos(), includeInvisible);
            entityTreeView.setSelectedEntity(selected);
        }
    }

    if (freeCamera) {
        handleFreeCameraInput(dt);
    }
}

void Game::handleFreeCameraInput(float dt)
{
    const auto& actionMapping = inputManager.getActionMapping();
    static const auto moveXAxis = actionMapping.getActionTagHash("CameraMoveX");
    static const auto moveYAxis = actionMapping.getActionTagHash("CameraMoveY");

    const auto moveStickState = util::getStickState(actionMapping, moveXAxis, moveYAxis);
    const auto cameraMoveSpeed = glm::vec2{240.f, 240.f};
    auto velocity = moveStickState * cameraMoveSpeed;
    if (inputManager.getKeyboard().isHeld(SDL_SCANCODE_LSHIFT)) {
        velocity *= 2.f;
    }

    auto cameraPos = gameCamera.getPosition2D();
    cameraPos += velocity * dt;
    gameCamera.setPosition2D(cameraPos);
}

void Game::devToolsUpdate(float dt)
{
    if (ImGui::Begin("Hello, world")) {
        using namespace devtools;
        BeginPropertyTable();
        const auto& mousePos = inputManager.getMouse().getPosition();
        const auto& gameScreenPos = edbr::util::
            getGameWindowScreenCoord(mousePos, gameWindowPos, gameWindowSize, params.renderSize);
        DisplayProperty("PlayerInput", playerInputEnabled);
        DisplayProperty("Mouse pos", mousePos);
        DisplayProperty("Game screen pos", gameScreenPos);
        const auto mouseWorldPos = getMouseWorldPos();
        DisplayProperty("Mouse wolrd pos", mouseWorldPos);
        DisplayProperty("Tile index", TileMap::GetTileIndex(mouseWorldPos));
        EndPropertyTable();
        ImGui::End();
    }

    if (ImGui::Begin("UI")) {
        ImGui::Checkbox("Draw UI bounding boxes", &uiInspector.drawUIElementBoundingBoxes);
        uiInspector.updateUI(dt);
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
        for (const auto&& [e, cc] : registry.view<CollisionComponent2D>().each()) {
            const auto bb = entityutil::getCollisionAABB({registry, e});
            auto collBoxColor = LinearColor{1.f, 0.f, 0.f, 0.5f};
            const auto& prefabName = registry.get<MetaInfoComponent>(e).prefabName;
            if (registry.all_of<TeleportComponent>(e) || prefabName == "trigger") {
                collBoxColor = LinearColor{1.f, 1.f, 0.f, 0.5f};
            }
            spriteRenderer.drawFilledRect(bb, collBoxColor);
        }
    }

    if (drawEntityTags) {
        for (const auto&& [e, tc] : registry.view<TagComponent>().each()) {
            // text is centered on top-center of entity rect
            const auto aabb = getSelectedEntityRect({registry, e});
            const auto textBB = devToolsFont.calculateTextBoundingBox(tc.tag);
            const auto textPos = (aabb.getPosition() + glm::vec2{aabb.width / 2.f, 0.f}) -
                                 glm::vec2{textBB.width / 2.f, textBB.height};

            // "shadow"
            spriteRenderer.drawText(
                devToolsFont, tc.tag, textPos + glm::vec2{1.f, 1.f}, LinearColor::Black());

            spriteRenderer.drawText(devToolsFont, tc.tag, textPos, LinearColor{1.f, 1.f, 0.f});
        }
    }
}
