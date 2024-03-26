#include "Game.h"

#include "Components.h"
#include "EntityUtil.h"

#include <edbr/DevTools/ImGuiPropertyTable.h>
#include <edbr/Util/Im3dUtil.h>
#include <edbr/Util/ImGuiUtil.h>

#include <im3d.h>
#include <imgui.h>

void Game::updateDevTools(float dt)
{
    if (displayFPSDelay > 0.f) {
        displayFPSDelay -= dt;
    } else {
        displayFPSDelay = 1.f;
        displayedFPS = avgFPS;
    }

    if (gameDrawnInWindow) {
        const auto windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize;
        if (ImGui::Begin(gameWindowLabel, nullptr, windowFlags)) {
            auto& drawImage = renderer.getDrawImage();
            const auto cursorPos = ImGui::GetCursorScreenPos();
            gameWindowPos = glm::ivec2{(int)cursorPos.x, (int)cursorPos.y};
            gameWindowSize = drawImage.getSize2D();
            util::ImGuiImage(drawImage);
        } else {
            gameWindowSize = glm::ivec2{};
        }
        ImGui::End();
    }

    if (ImGui::Begin("Debug")) {
        ImGui::Text("FPS: %d", (int)displayedFPS);
        ImGui::Checkbox("Draw game in window", &gameDrawnInWindow);
        ImGui::Checkbox("Draw entity tags", &drawEntityTags);
        ImGui::Checkbox("Local gizmo", &Im3d::GetContext().m_gizmoLocal);

        /* static const auto interactTipImageId =
            gfxDevice.loadImageFromFile("assets/images/ui/interact_tip.png");
        static const auto talkTipImageId =
            gfxDevice.loadImageFromFile("assets/images/ui/talk_tip.png");

        util::ImGuiImage(gfxDevice, interactTipImageId);
        ImGui::SameLine();
        util::ImGuiImage(gfxDevice, talkTipImageId); */

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

        if (ImGui::CollapsingHeader("Scene settings")) {
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
        }

        if (ImGui::CollapsingHeader("Renderer settings")) {
            renderer.updateDevTools(dt);
        }
        ui.updateDevTools(dt);
    }
    ImGui::End();

    if (ImGui::Begin("Entities")) {
        entityTreeView.update(registry, dt);
    }
    ImGui::End();

    const auto& imageCache = gfxDevice.getImageCache();
    const auto& materialCache = renderer.getBaseRenderer().getMaterialCache();
    resourcesInspector.update(dt, imageCache, materialCache);

    if (entityTreeView.hasSelectedEntity()) {
        if (ImGui::Begin("Selected entity")) {
            entityInfoDisplayer.displayEntityInfo(entityTreeView.getSelectedEntity(), dt);
        }
        ImGui::End();
    }

    auto e = entityutil::getPlayerEntity(registry);
    const auto& tc = e.get<TransformComponent>();
    const auto& pos = tc.transform.getPosition();

    Im3d::PushLayerId(Im3dState::WorldNoDepthLayer);
    Im3dDrawArrow(glm::vec4{1.f, 0.f, 1.f, 0.5f}, pos, pos + tc.transform.getLocalFront() * 0.5f);
    // Im3d::DrawCapsule(glm2im3d(pos), glm2im3d(pos + glm::vec3{0.f, 1.f, 0.f}), 0.5f, 50);

    Im3d::Mat4 transform = glm2im3d(tc.worldTransform);
    if (Im3d::Gizmo("Gizmo", (float*)&transform)) {
    }

    Im3d::PopLayerId();

    // draw tags
    if (drawEntityTags) {
        for (const auto& [e, tc, tagC] : registry.view<TransformComponent, TagComponent>().each()) {
            if (!tagC.getTag().empty()) {
                Im3dText(tc.transform.getPosition(), 1.f, glm::vec4{1.f}, tagC.getTag().c_str());
            }
        }
    }

    ImGui::ShowDemoWindow();
}
