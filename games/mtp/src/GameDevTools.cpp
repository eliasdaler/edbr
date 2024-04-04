#include "Game.h"

#include "Components.h"
#include "EntityUtil.h"

#include <edbr/DevTools/ImGuiPropertyTable.h>
#include <edbr/Graphics/CoordUtil.h>
#include <edbr/Util/Im3dUtil.h>
#include <edbr/Util/ImGuiUtil.h>

#include <im3d.h>
#include <imgui.h>

namespace
{
void drawFrustum(const Camera& camera)
{
    const auto corners = edge::calculateFrustumCornersWorldSpace(camera);

    // near plane
    Im3d::PushColor(Im3d::Color_Magenta);
    Im3d::DrawQuad(
        glm2im3d(glm::vec3{corners[0]}),
        glm2im3d(glm::vec3{corners[1]}),
        glm2im3d(glm::vec3{corners[2]}),
        glm2im3d(glm::vec3{corners[3]}));
    Im3d::PopColor();

    // far plane
    Im3d::PushColor(Im3d::Color_Orange);
    Im3d::DrawQuad(
        glm2im3d(glm::vec3{corners[4]}),
        glm2im3d(glm::vec3{corners[5]}),
        glm2im3d(glm::vec3{corners[6]}),
        glm2im3d(glm::vec3{corners[7]}));
    Im3d::PopColor();

    // left plane
    Im3d::DrawQuad(
        glm2im3d(glm::vec3{corners[4]}),
        glm2im3d(glm::vec3{corners[5]}),
        glm2im3d(glm::vec3{corners[1]}),
        glm2im3d(glm::vec3{corners[0]}));

    // right plane
    Im3d::DrawQuad(
        glm2im3d(glm::vec3{corners[7]}),
        glm2im3d(glm::vec3{corners[6]}),
        glm2im3d(glm::vec3{corners[2]}),
        glm2im3d(glm::vec3{corners[3]}));

    for (std::size_t i = 0; i < corners.size(); ++i) {
        Im3dText(corners[i], 8.f, RGBColor{255, 255, 255, 255}, std::to_string(i).c_str());
    }
}
}

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
        ImGui::Checkbox("Draw entity heading", &drawEntityHeading);
        ImGui::Checkbox("Draw gizmo", &drawGizmos);
        ImGui::Text("Char on ground: %d", (int)physicsSystem->isCharacterOnGround());

        if (drawGizmos) {
            ImGui::Checkbox("Local gizmo", &Im3d::GetContext().m_gizmoLocal);
            int gizmoMode = (int)Im3d::GetContext().m_gizmoMode;
            ImGui::RadioButton("Translate (Ctrl+T)", &gizmoMode, Im3d::GizmoMode_Translation);
            ImGui::SameLine();
            ImGui::RadioButton("Rotate (Ctrl+R)", &gizmoMode, Im3d::GizmoMode_Rotation);
            ImGui::SameLine();
            ImGui::RadioButton("Scale (Ctrl+S)", &gizmoMode, Im3d::GizmoMode_Scale);
            Im3d::GetContext().m_gizmoMode = (Im3d::GizmoMode)gizmoMode;
        }

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

            const auto& mousePos = inputManager.getMouse().getPosition();
            const auto& gameScreenPos = edbr::util::getGameWindowScreenCoord(
                mousePos, gameWindowPos, gameWindowSize, {params.renderWidth, params.renderHeight});
            DisplayProperty("Mouse pos", mousePos);
            DisplayProperty("Game screen pos", gameScreenPos);
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

    if (ImGui::Begin("Camera")) {
        using namespace devtools;
        BeginPropertyTable();
        DisplayProperty("Cam pos", camera.getPosition());
        DisplayProperty("Cam heading", camera.getHeading());
        EndPropertyTable();

        bool recreateCamera = false;
        float fovXDeg = glm::degrees(cameraFovX);
        if (ImGui::DragFloat("fovX", &fovXDeg, 0.1f, 10.f, 120.f)) {
            cameraFovX = glm::radians(fovXDeg);
            recreateCamera = true;
        }
        if (ImGui::DragFloat("near", &cameraNear, 0.1f, 0.01f, 100.f)) {
            recreateCamera = true;
        }
        if (ImGui::DragFloat("far", &cameraFar, 0.1f, 10.f, 10000.f)) {
            recreateCamera = true;
        }
        if (recreateCamera) {
            static const float aspectRatio = (float)params.renderWidth / (float)params.renderHeight;

            camera.init(cameraFovX, cameraNear, cameraFar, aspectRatio);
        }

        ImGui::Checkbox("Smoothing", &smoothCamera);
        ImGui::Checkbox("Smart smoothing", &smoothSmartCamera);
        ImGui::Checkbox("Draw track point", &drawCameraTrackPoint);
        ImGui::DragFloat("Delay", &cameraDelay, 0.1f, 0.f, 1.f);
        ImGui::DragFloat("Max speed", &cameraMaxSpeed, 0.1f, 10.f, 1000.f);
        ImGui::DragFloat("Y offset", &cameraYOffset, 0.1f);
        ImGui::DragFloat("Z offset", &cameraZOffset, 0.1f);
        ImGui::DragFloat("max offset time", &cameraMaxOffsetTime, 0.1f);
        ImGui::DragFloat("max offset run", &maxCameraOffsetFactorRun, 0.1f);
        ImGui::DragFloat("Test", &testParam, 0.1f);
        ImGui::Text(".%2f", prevOffsetZ);

        ImGui::DragFloat("Yaw rotate speed", &cameraController.rotateYawSpeed, 0.1f);
        ImGui::DragFloat("Pitch rotate speed", &cameraController.rotatePitchSpeed, 0.1f);
    }
    ImGui::End();

    if (ImGui::Begin("Entities")) {
        entityTreeView.update(registry, dt);
    }
    ImGui::End();

    const auto& imageCache = gfxDevice.getImageCache();
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
    if (drawEntityHeading) {
        Im3dDrawArrow(RGBColor{255, 0, 255, 128}, pos, pos + tc.transform.getLocalFront() * 0.5f);
    }

    // Im3d::DrawCapsule(glm2im3d(pos), glm2im3d(pos + glm::vec3{0.f, 1.f, 0.f}), 0.5f, 50);

    if (drawGizmos && entityTreeView.hasSelectedEntity()) {
        auto ent = entityTreeView.getSelectedEntity().entity();
        auto e = entt::handle{registry, ent};
        auto& tc = e.get<TransformComponent>();
        Im3d::Mat4 transform = glm2im3d(tc.worldTransform);
        if (Im3d::Gizmo("Gizmo", (float*)&transform)) {
            tc.transform = Transform(im3d2glm(transform));
        }
    }

    Im3d::PopLayerId();

    if (drawEntityTags) {
        for (const auto& [e, tc, tagC] : registry.view<TransformComponent, TagComponent>().each()) {
            if (!tagC.getTag().empty()) {
                Im3dText(
                    tc.transform.getPosition(),
                    1.f,
                    RGBColor{255, 255, 255},
                    tagC.getTag().c_str());
            }
        }
    }

    physicsSystem->updateDevUI(inputManager, dt);

    ImGui::ShowDemoWindow();
}
