#include "Game.h"

#include <edbr/DevTools/ImGuiPropertyTable.h>
#include <edbr/Util/ImGuiUtil.h>
#include <imgui.h>

void Game::updateDevTools(float dt)
{
    if (displayFPSDelay > 0.f) {
        displayFPSDelay -= dt;
    } else {
        displayFPSDelay = 1.f;
        displayedFPS = avgFPS;
    }

    if (ImGui::Begin("Debug")) {
        ImGui::Text("FPS: %d", (int)displayedFPS);

        static const auto interactTipImageId =
            gfxDevice.loadImageFromFile("assets/images/ui/interact_tip.png");
        static const auto talkTipImageId =
            gfxDevice.loadImageFromFile("assets/images/ui/talk_tip.png");

        util::ImGuiImage(gfxDevice, interactTipImageId);
        ImGui::SameLine();
        util::ImGuiImage(gfxDevice, talkTipImageId);

        using namespace devtools;
        BeginPropertyTable();
        {
            DisplayProperty("Level", level.getPath().string());

            const auto cameraPos = camera.getPosition();
            DisplayProperty("Cam pos", cameraPos);
            DisplayProperty("Cam heading", camera.getHeading());
        }
        EndPropertyTable();

        ImGui::Checkbox("Orbit around selected entity", &orbitCameraAroundSelectedEntity);
        ImGui::DragFloat("Orbit distance", &orbitDistance, 0.01f, 0.f, 100.f);

        auto ambientColor = level.getAmbientLightColor();
        if (util::ImGuiColorEdit3("Ambient", ambientColor)) {
            level.setAmbientLightColor(ambientColor);
        }
        auto ambientIntensity = level.getAmbientLightIntensity();
        if (ImGui::DragFloat("Ambient intensity", &ambientIntensity, 1.f, 0.f, 1.f)) {
            level.setAmbientLightIntensity(ambientIntensity);
        }

        auto fogColor = level.getFogColor();
        if (util::ImGuiColorEdit3("Fog", fogColor)) {
            level.setFogColor(fogColor);
        }
        auto fogDensity = level.getFogDensity();
        if (ImGui::DragFloat("Fog density", &fogDensity, 1.f, 0.f, 1.f)) {
            level.setFogDensity(fogDensity);
        }

        renderer.updateDevTools(dt);
        ui.updateDevTools(dt);
    }
    ImGui::End();

    if (ImGui::Begin("Entities")) {
        entityTreeView.update(registry, dt);
    }
    ImGui::End();

    if (entityTreeView.hasSelectedEntity()) {
        if (ImGui::Begin("Selected entity")) {
            entityInfoDisplayer.displayEntityInfo(entityTreeView.getSelectedEntity(), dt);
        }
        ImGui::End();
    }

    // ImGui::ShowDemoWindow();
}
