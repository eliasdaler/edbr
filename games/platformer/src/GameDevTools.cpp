#include "Game.h"

#include <edbr/DevTools/ImGuiPropertyTable.h>
#include <edbr/Util/ImGuiUtil.h>
#include <imgui.h>

void Game::devToolsHandleInput(float dt)
{
    if (inputManager.getKeyboard().wasJustPressed(SDL_SCANCODE_C)) {
        freeCamera = !freeCamera;
    }
}

void Game::devToolsUpdate(float dt)
{
    if (ImGui::Begin("Hello, world")) {
        using namespace devtools;
        BeginPropertyTable();
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
