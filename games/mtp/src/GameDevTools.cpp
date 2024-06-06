#include "Game.h"

#include "Components.h"
#include "EntityUtil.h"
#include "FollowCameraController.h"

#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/ECS/Components/NPCComponent.h>
#include <edbr/ECS/Components/SceneComponent.h>
#include <edbr/ECS/Components/TagComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>

#include <edbr/DevTools/ImGuiPropertyTable.h>

#include <edbr/Graphics/CoordUtil.h>
#include <edbr/Graphics/SpriteRenderer.h>
#include <edbr/Util/Im3dUtil.h>
#include <edbr/Util/ImGuiUtil.h>

#include <im3d.h>
#include <imgui.h>

namespace
{
const char* gameStateStr(Game::GameState state)
{
    switch (state) {
    case Game::GameState::Invalid:
        return "Invalid";
    case Game::GameState::MainMenu:
        return "MainMenu";
    case Game::GameState::Playing:
        return "Playing";
    case Game::GameState::Cutscene:
        return "Cutscene";
    case Game::GameState::Paused:
        return "Paused";
    default:
        assert(false);
        return "";
    }
}
}

void Game::devToolsNewFrame(float dt)
{
    // im3d
    {
        const auto& mouse = inputManager.getMouse();
        bool isMousePressed = mouse.isHeld(SDL_BUTTON(1));
        const auto gameScreenMousePos = getMouseGameScreenCoord();
        im3d.newFrame(
            dt,
            static_cast<glm::vec2>(getGameScreenSize()),
            camera,
            static_cast<glm::ivec2>(gameScreenMousePos),
            isMousePressed);
        // Note: this passes previous frame's camera position/orientation
        // We want to call im3d.newFrame before the game's update so that we
        // can draw debug shapes from anywhere, not just devToolsUpdate.
    }
}

void Game::devToolsHandleInput(float dt)
{
    const auto& kb = inputManager.getKeyboard();
    if (kb.wasJustPressed(SDL_SCANCODE_T)) {
        drawEntityTags = !drawEntityTags;
    }

    // spawn ball when pressing "O"
    if (kb.wasJustPressed(SDL_SCANCODE_O)) {
        static std::default_random_engine randEngine(1337);
        static std::uniform_real_distribution<float> scaleDist(2.f, 3.f);
        auto ball = entityCreator.createFromPrefab("ball");

        // random scale
        const auto scale = scaleDist(randEngine);
        auto& transform = ball.get<TransformComponent>().transform;
        transform.setScale(glm::vec3{scale});

        // sync physics
        physicsSystem->updateTransform(ball.get<PhysicsComponent>().bodyId, transform, true);
        // set velocity
        physicsSystem->setVelocity(
            ball.get<PhysicsComponent>().bodyId, camera.getTransform().getLocalFront() * 20.f);
        // set position
        entityutil::teleportEntity(ball, camera.getPosition());
    }

    if (kb.wasJustPressed(SDL_SCANCODE_B)) {
        physicsSystem->drawCollisionShapes = !physicsSystem->drawCollisionShapes;
    }

    if (kb.wasJustPressed(SDL_SCANCODE_M)) {
        audioManager.setMuted(!audioManager.isMuted());
    }

    if (kb.wasJustPressed(SDL_SCANCODE_P)) {
        devPaused = !devPaused;
    }

    if (kb.wasJustPressed(SDL_SCANCODE_LEFTBRACKET)) {
        devPaused = false;
        devResumeForOneFrame = true;
    }

    if (kb.isHeld(SDL_SCANCODE_RIGHTBRACKET)) {
        devPaused = false;
        devResumeForOneFrame = true;
    }

    const auto& actionMapping = inputManager.getActionMapping();
    // "C" by default
    static const auto toggleFreeCameraAction = actionMapping.getActionTagHash("ToggleFreeCamera");
    if (actionMapping.wasJustPressed(toggleFreeCameraAction) &&
        entityutil::playerExists(registry)) {
        freeCameraMode = !freeCameraMode;
        playerInputEnabled = !freeCameraMode;
        if (!freeCameraMode) {
            if (previousCameraControllerTag == followCameraControllerTag) {
                auto& controller = getFollowCameraController();
                controller.init(camera);
                controller.teleportCameraToDesiredPosition(camera);
            }
            camera.setPosition(previousCameraTransform.getPosition());
            camera.setHeading(previousCameraTransform.getHeading());
            cameraManager.setController(previousCameraControllerTag);
        } else {
            previousCameraControllerTag = cameraManager.getCurrentControllerTag();
            previousCameraTransform = camera.getTransform();
            cameraManager.setController(freeCameraControllerTag);
            cameraManager.stopCameraTransitions();
        }
    }

    // "Tab" by default
    static const auto toggleImGuiAction = actionMapping.getActionTagHash("ToggleImGuiVisibility");
    if (actionMapping.wasJustPressed(toggleImGuiAction)) {
        drawImGui = !drawImGui;
    }
}

void Game::devToolsUpdate(float dt)
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
        if (devPaused) {
            if (ImGui::Button("Resume")) {
                devPaused = false;
            }
        } else {
            if (ImGui::Button("Pause")) {
                devPaused = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Advance frame")) {
            devResumeForOneFrame = true;
            devPaused = false;
        }

        ImGui::Text("FPS: %d", (int)displayedFPS);

        ImGui::Checkbox("Draw game in window", &gameDrawnInWindow);
        ImGui::Checkbox("Draw entity tags", &drawEntityTags);
        ImGui::Checkbox("Draw entity heading", &drawEntityHeading);
        ImGui::Checkbox("Draw gizmo", &drawGizmos);

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

        using namespace devtools;
        BeginPropertyTable();
        {
            DisplayProperty("Game state", gameStateStr(gameState));
            DisplayProperty("Game paused", gamePaused);
            DisplayProperty("Input enabled", playerInputEnabled);
            DisplayProperty("Level", level.getPath().string());
            DisplayProperty("Level name", level.getName());

            const auto& mousePos = inputManager.getMouse().getPosition();
            const auto& gameScreenPos = edbr::util::getGameWindowScreenCoord(
                mousePos, gameWindowPos, gameWindowSize, params.renderSize);
            DisplayProperty("Mouse pos", mousePos);
            DisplayProperty("Game screen pos", gameScreenPos);
            DisplayProperty("Game screen pos", gameScreenPos);
        }
        EndPropertyTable();

        ImGui::Checkbox("Orbit around selected entity", &orbitCameraAroundSelectedEntity);

        if (ImGui::CollapsingHeader("Save files")) {
            if (ImGui::Button("Save")) {
                saveFileManager.writeCurrentSaveFile();
            }
            BeginPropertyTable();
            DisplayProperty("Save index", saveFileManager.getCurrentSaveFileIndex());
            DisplayProperty(
                "Save exits",
                saveFileManager.saveFileExists(saveFileManager.getCurrentSaveFileIndex()));
            DisplayProperty("Save path", saveFileManager.getCurrentSaveFilePath().string());
            EndPropertyTable();

            if (ImGui::TreeNode("Raw data")) {
                BeginPropertyTable();
                for (auto& [key, val] : saveFileManager.getCurrentSaveFile().getRawData().items()) {
                    if (val.is_string()) {
                        DisplayProperty(key.c_str(), val.get<std::string>());
                    } else if (val.is_number()) {
                        DisplayProperty(key.c_str(), val.get<float>());
                    } else if (val.is_boolean()) {
                        DisplayProperty(key.c_str(), val.get<bool>());
                    } else {
                        DisplayProperty(key.c_str(), "???");
                    }
                }
                EndPropertyTable();
                ImGui::TreePop();
            }
        }

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
                level.setFogActive(fogDensity != 0.f);
            }
        }

        if (ImGui::CollapsingHeader("Renderer settings")) {
            renderer.updateDevTools(dt);
        }
    }
    ImGui::End();

    if (ImGui::Begin("UI")) {
        ui.updateDevTools(dt);
    }
    ImGui::End();

    if (ImGui::Begin("Action lists")) {
        for (const auto& [name, al] : actionListManager.getActionLists()) {
            actionListInspector.showActionListInfo(al, true);
            ImGui::Separator();
        }
    }
    ImGui::End();

    if (ImGui::Begin("Camera")) {
        using namespace devtools;
        BeginPropertyTable();
        DisplayProperty("Camera controller", cameraManager.getCurrentControllerTag());
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
            static const float aspectRatio =
                (float)params.renderSize.x / (float)params.renderSize.y;

            camera.init(cameraFovX, cameraNear, cameraFar, aspectRatio);
        }

        auto& fcc = getFollowCameraController();
        ImGui::Checkbox("Smart smoothing", &fcc.smoothSmartCamera);
        ImGui::Checkbox("Draw track point", &fcc.drawCameraTrackPoint);
        ImGui::DragFloat("Delay", &fcc.cameraDelay, 0.1f, 0.f, 1.f);
        ImGui::DragFloat("Max speed", &fcc.cameraMaxSpeed, 0.1f, 10.f, 1000.f);
        ImGui::DragFloat("Y offset", &fcc.cameraYOffset, 0.1f);
        ImGui::DragFloat("Z offset", &fcc.cameraZOffset, 0.1f);
        ImGui::DragFloat("Orbit distance", &fcc.orbitDistance, 0.1f);
        ImGui::DragFloat("Yaw", &fcc.currentYaw, 0.1f);
        ImGui::DragFloat("Pitch", &fcc.currentPitch, 0.1f);
        ImGui::Checkbox("Control rotation", &fcc.controlRotation);
        ImGui::DragFloat("max offset time", &fcc.cameraMaxOffsetTime, 0.1f);
        ImGui::DragFloat("max offset run", &fcc.maxCameraOffsetFactorRun, 0.1f);
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
            auto selectedEntity = entityTreeView.getSelectedEntity();
            if (ImGui::Button("Destroy")) {
                entityTreeView.deselectedEntity();
                destroyEntity(selectedEntity);
                selectedEntity = {};
            }
            if (selectedEntity.entity() != entt::null) {
                entityInfoDisplayer.displayEntityInfo(selectedEntity, dt);
            }
        }
        ImGui::End();
    }

    if (auto player = entityutil::getPlayerEntity(registry); player.entity() != entt::null) {
        const auto& tc = player.get<TransformComponent>();
        const auto& pos = tc.transform.getPosition();

        Im3d::PushLayerId(Im3dState::WorldNoDepthLayer);
        if (drawEntityHeading) {
            Im3dDrawArrow(
                RGBColor{255, 0, 255, 128}, pos, pos + tc.transform.getLocalFront() * 0.5f);
        }
        Im3d::PopLayerId();
    }

    // Im3d::DrawCapsule(glm2im3d(pos), glm2im3d(pos + glm::vec3{0.f, 1.f, 0.f}), 0.5f, 50);

    if (drawGizmos && entityTreeView.hasSelectedEntity()) {
        Im3d::PushLayerId(Im3dState::WorldNoDepthLayer);
        auto ent = entityTreeView.getSelectedEntity().entity();
        auto e = entt::handle{registry, ent};
        auto& tc = e.get<TransformComponent>();
        Im3d::Mat4 transform = glm2im3d(tc.worldTransform);
        if (Im3d::Gizmo("Gizmo", (float*)&transform)) {
            tc.transform = Transform(im3d2glm(transform));
        }
        Im3d::PopLayerId();
    }

    if (drawEntityTags || physicsSystem->drawCollisionShapes) {
        for (auto entity : registry.view<entt::entity>()) {
            auto e = entt::const_handle(registry, entity);
            const auto& metaName = entityutil::getMetaName(e);
            if (e.any_of<TagComponent, NameComponent>() && !metaName.empty()) {
                auto& sc = registry.get<SceneComponent>(e);
                if (e.any_of<
                        PlayerSpawnComponent,
                        ColliderComponent,
                        TriggerComponent,
                        NPCComponent>()) {
                    // it's easier to read Collision./Interact./Trigger.<Something>
                    // instead of just "Something"
                    Im3dText(
                        e.get<TransformComponent>().transform.getPosition(),
                        1.f,
                        RGBColor{255, 255, 255},
                        sc.sceneNodeName.c_str());
                } else {
                    Im3dText(
                        e.get<TransformComponent>().transform.getPosition(),
                        1.f,
                        RGBColor{255, 255, 255},
                        metaName.c_str());
                }
            }
        }
    }

    physicsSystem->drawDebugShapes(camera);
    physicsSystem->updateDevUI(inputManager, dt);

    // draw spawners and cameras
    if (physicsSystem->drawCollisionShapes) {
        Im3d::PushLayerId(Im3dState::WorldNoDepthLayer);
        for (const auto& [e, tc, nc] : registry.view<TransformComponent, NameComponent>().each()) {
            if (!registry.any_of<CameraComponent, PlayerSpawnComponent>(e)) {
                continue;
            }
            const auto& transform = tc.transform;

            if (registry.all_of<PlayerSpawnComponent>(e)) {
                static float arrowLength = 0.5f;
                Im3dDrawArrow(
                    RGBColor{255, 0, 0},
                    transform.getPosition(),
                    transform.getPosition() + transform.getLocalRight() * arrowLength);
                Im3dDrawArrow(
                    RGBColor{0, 255, 0},
                    transform.getPosition(),
                    transform.getPosition() + transform.getLocalUp() * arrowLength);
                Im3dDrawArrow(
                    RGBColor{0, 0, 255},
                    transform.getPosition(),
                    transform.getPosition() + transform.getLocalFront() * arrowLength);
            }

            if (registry.all_of<CameraComponent>(e)) {
                Camera camera;
                camera.setPosition(transform.getPosition());
                camera.setHeading(transform.getHeading());
                static const float aspectRatio =
                    (float)params.renderSize.x / (float)params.renderSize.y;

                camera.init(cameraFovX, 0.001f, 1.f, aspectRatio);
                Im3dDrawFrustum(camera);
            }
        }
        Im3d::PopLayerId();
    }

    ImGui::ShowDemoWindow();

    // im3d end
    im3d.updateCameraViewProj(Im3dState::WorldNoDepthLayer, camera.getViewProj());
    im3d.updateCameraViewProj(Im3dState::WorldWithDepthLayer, camera.getViewProj());
    im3d.endFrame();
    im3d.drawText(
        camera.getViewProj(), gameWindowPos, gameWindowSize, gameWindowLabel, gameDrawnInWindow);
}

void Game::devToolsDrawUI(SpriteRenderer& spriteRenderer)
{
    if (devPaused) {
        spriteRenderer.drawInsetRect(
            {{}, static_cast<glm::vec2>(getGameScreenSize())}, LinearColor{1.f, 1.f, 1.f}, 4.f);
        spriteRenderer
            .drawText(ui.getDefaultFont(), "Paused", {32.f, 32.f}, LinearColor{1.f, 1.f, 1.f});
    }
    if (freeCameraMode) {
        // spriteRenderer
        //     .drawText(ui.getDefaultFont(), "Free cam", {32.f, 64.f}, LinearColor{1.f, 1.f, 1.f});
    }
}
