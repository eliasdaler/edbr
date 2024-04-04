#include "Game.h"

#include "Components.h"
#include "EntityUtil.h"
#include "GameSceneLoader.h"
#include "Systems.h"

#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/Graphics/CoordUtil.h>
#include <edbr/Graphics/Cubemap.h>
#include <edbr/Graphics/Letterbox.h>
#include <edbr/Graphics/Scene.h>
#include <edbr/Graphics/Vulkan/Util.h>
#include <edbr/Math/Util.h>
#include <edbr/Util/FS.h>
#include <edbr/Util/InputUtil.h>

#include <tracy/Tracy.hpp>

#include <edbr/Util/Im3dUtil.h>
#include <im3d.h>
#include <imgui.h>

#include <glm/gtx/easing.hpp>

namespace eu = entityutil;

Game::Game() :
    Application(),
    renderer(gfxDevice, meshCache, materialCache),
    spriteRenderer(gfxDevice),
    sceneCache(animationCache),
    entityInitializer(sceneCache, gfxDevice, meshCache, materialCache, animationCache),
    animationSoundSystem(audioManager)
{}

namespace
{
glm::vec3 getCameraOrbitTarget(entt::const_handle e, float cameraYOffset, float cameraZOffset)
{
    const auto& tc = e.get<TransformComponent>();
    return tc.transform.getPosition() + math::GlobalUpAxis * cameraYOffset +
           tc.transform.getLocalFront() * cameraZOffset;
}

glm::vec3 getCameraDesiredPosition(
    const Camera& camera,
    const glm::vec3& trackPoint,
    float orbitDistance)
{
    return trackPoint - camera.getTransform().getLocalFront() * orbitDistance;
}
}

void Game::customInit()
{
    inputManager.getActionMapping().loadActions("assets/data/input_actions.json");
    inputManager.loadMapping("assets/data/input_mapping.json");
    animationCache.loadAnimationData("assets/data/animation_data.json");

    materialCache.init(gfxDevice);
    renderer.init(glm::ivec2{params.renderWidth, params.renderHeight});
    spriteRenderer.init(renderer.getDrawImageFormat());

    // important: need to do this before creating any Jolt objects
    PhysicsSystem::InitStaticObjects();
    physicsSystem = std::make_unique<PhysicsSystem>();
    physicsSystem->init();

    animationSoundSystem.init(eventManager, "assets/sounds");

    skyboxDir = "assets/images/skybox";

    { // im3d
        im3d.init(gfxDevice, renderer.getDrawImageFormat(), renderer.getDepthImageFormat());
        im3d.addRenderState(
            Im3dState::DefaultLayer,
            Im3dState::RenderState{.depthTest = false, .viewProj = glm::mat4{1.f}});
        im3d.addRenderState(
            Im3dState::WorldNoDepthLayer,
            Im3dState::RenderState{.depthTest = false, .viewProj = glm::mat4{1.f}});
        im3d.addRenderState(
            Im3dState::WorldWithDepthLayer,
            Im3dState::RenderState{.depthTest = true, .viewProj = glm::mat4{1.f}});
    }

    initEntityFactory();
    registerComponents(entityFactory.getComponentFactory());
    registerComponentDisplayers();

    ui.init(gfxDevice);

    { // create camera
        static const float aspectRatio = (float)params.renderWidth / (float)params.renderHeight;

        camera.setUseInverseDepth(true);
        camera.init(cameraFovX, cameraNear, cameraFar, aspectRatio);
    }

    { // create player
        auto e = entityFactory.createEntity(registry, "cato");
        e.emplace<PlayerComponent>();
    }

    // loadLevel("assets/levels/house.json");
    loadLevel("assets/levels/city.json");

    // spawn player
    const auto& spawnName = level.getDefaultPlayerSpawnerName();
    if (!spawnName.empty()) {
        eu::spawnPlayer(registry, level.getDefaultPlayerSpawnerName());
        util::onPlaceEntityOnScene(createSceneLoadContext(), eu::getPlayerEntity(registry));
        // FIXME: physics system should catch position change event
        physicsSystem->setCharacterPosition(eu::getWorldPosition(eu::getPlayerEntity(registry)));
    }

    // set camera
    const auto& cameraName = level.getDefaultCameraName();
    if (!cameraName.empty()) {
        setCurrentCamera(eu::findCameraByName(registry, cameraName));
    } else {
        auto player = eu::getPlayerEntity(registry);
        if (player.entity() != entt::null) {
            fmt::println("No default camera found: using follow camera");

            orbitCameraAroundSelectedEntity = true;

            // follow cam
            camera.setYawPitch(glm::radians(-135.f), glm::radians(-20.f));

            // FIXME: should be done more automatically?
            // e.g. setFollowEntity(camera, player, Behaviour::TeleportInstantly)
            cameraCurrentTrackPointPos = getCameraOrbitTarget(player, cameraYOffset, cameraZOffset);
            camera.setPosition(
                getCameraDesiredPosition(camera, cameraCurrentTrackPointPos, orbitDistance));

            entityTreeView.setSelectedEntity(player);
        } else {
            // try to find camera named "Default"
            auto cameraEnt = findDefaultCamera();
            if (cameraEnt.entity() != entt::null) {
                setCurrentCamera(cameraEnt);
            } else {
                fmt::println("No default camera found");
            }
        }
    }

    { // create entities from prefabs
        { // test
            /* const glm::vec3 treePos{-58.5f, 0.07f, 7.89f};
            auto tree = entityFactory.createEntity(registry, "pine_tree");
            eu::setPosition(tree, treePos);
            physicsSystem->updateTransform(
                tree.get<PhysicsComponent>().bodyId, tree.get<TransformComponent>().transform);
            setEntityTag(tree, "WiseTree");

            auto interactor = entityFactory.createEntity(registry, "interaction_trigger");
            eu::addChild(tree, interactor); */
        }

        { // for city level
            auto npc = eu::findEntityBySceneNodeName(registry, "generic_npc.2");
            if ((bool)npc) {
                setEntityTag(npc, "CoolDude");
            }
        }
    }
}

void Game::customUpdate(float dt)
{
    ZoneScopedN("Update");

    auto& io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard) {
        handleInput(dt);
    }
    if (!gameDrawnInWindow) {
        const auto blitRect = util::
            calculateLetterbox(renderer.getDrawImage().getSize2D(), gfxDevice.getSwapchainSize());
        gameWindowPos = {blitRect.x, blitRect.y};
        gameWindowSize = {blitRect.z, blitRect.w};
    }

    // im3d
    {
        const auto internalSize = glm::vec2{params.renderWidth, params.renderHeight};
        const auto& mouse = inputManager.getMouse();
        bool isMousePressed = mouse.isHeld(SDL_BUTTON(1));
        const auto& gameScreenPos = edbr::util::getGameWindowScreenCoord(
            mouse.getPosition(),
            gameWindowPos,
            gameWindowSize,
            {params.renderWidth, params.renderHeight});
        im3d.newFrame(dt, internalSize, camera, gameScreenPos, isMousePressed);
    }

    // movement
    movementSystemUpdate(registry, dt);
    { // physics
        auto player = entityutil::getPlayerEntity(registry);
        physicsSystem->drawDebugShapes(camera);
        physicsSystem->update(dt, player.get<TransformComponent>().transform.getHeading());
        { // sync player with the virtual character
            if (player.entity() != entt::null) {
                eu::setPosition(player, physicsSystem->getCharacterPosition());
            }
        }
        // sync other entities with their physics bodies transforms
        auto physicsView = registry.view<TransformComponent, PhysicsComponent>();
        for (auto&& [e, tc, pc] : physicsView.each()) {
            physicsSystem->syncVisibleTransform(pc.bodyId, tc.transform);
        }
    }

    transformSystemUpdate(registry, dt);
    movementSystemPostPhysicsUpdate(registry, dt);
    playerAnimationSystem(registry, *physicsSystem, dt);
    skeletonAnimationSystemUpdate(registry, eventManager, dt);

    // camera update
    cameraController.update(camera, dt);
    if (orbitCameraAroundSelectedEntity && entityTreeView.hasSelectedEntity()) {
        updateFollowCamera(entityTreeView.getSelectedEntity(), dt);
    }

    // audio needs to be updated after the camera
    animationSoundSystem.update(registry, camera, dt);

    updateDevTools(dt);

    //  needs to be called in the end of update after the camera is in the final position
    im3d.updateCameraViewProj(Im3dState::WorldNoDepthLayer, camera.getViewProj());
    im3d.updateCameraViewProj(Im3dState::WorldWithDepthLayer, camera.getViewProj());

    im3d.endFrame();

    { // draw im3d text
        if (gameDrawnInWindow) {
            ImGui::Begin(gameWindowLabel);
            im3d.drawText(camera.getViewProj(), gameWindowPos, gameWindowSize);
            ImGui::End();
        } else {
            // TODO: get proper window coords
            ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32_BLACK_TRANS);
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImVec2{
                (float)gfxDevice.getSwapchainExtent().width,
                (float)gfxDevice.getSwapchainExtent().height});

            ImGui::Begin(
                "Invisible",
                nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs |
                    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                    ImGuiWindowFlags_NoBringToFrontOnFocus);
            im3d.drawText(camera.getViewProj(), gameWindowPos, gameWindowSize);
            ImGui::End();
            ImGui::PopStyleColor(1);
        }
    }
}

void Game::handleInput(float dt)
{
    cameraController.handleInput(inputManager, camera);

    const auto& actionMapping = inputManager.getActionMapping();
    static const auto toggleFreeCameraAction = actionMapping.getActionTagHash("ToggleFreeCamera");
    if (actionMapping.wasJustPressed(toggleFreeCameraAction)) {
        orbitCameraAroundSelectedEntity = !orbitCameraAroundSelectedEntity;
    }

    static const auto toggleImGuiAction = actionMapping.getActionTagHash("ToggleImGuiVisibility");
    if (actionMapping.wasJustPressed(toggleImGuiAction)) {
        drawImGui = !drawImGui;
    }

    if (orbitCameraAroundSelectedEntity) {
        handlePlayerInput(dt);
    }

    const auto& kb = inputManager.getKeyboard();
    if (kb.wasJustPressed(SDL_SCANCODE_T)) {
        drawEntityTags = !drawEntityTags;
    }

    if (kb.wasJustPressed(SDL_SCANCODE_O)) {
        static std::default_random_engine randEngine(1337);
        static std::uniform_real_distribution<float> scaleDist(2.f, 3.f);
        auto ball = entityFactory.createEntity(registry, "ball");
        eu::setPosition(ball, camera.getPosition());
        const auto scale = scaleDist(randEngine);
        ball.get<TransformComponent>().transform.setScale(glm::vec3{scale});
        util::onPlaceEntityOnScene(createSceneLoadContext(), ball);
        physicsSystem->setVelocity(
            ball.get<PhysicsComponent>().bodyId, camera.getTransform().getLocalFront() * 20.f);
    }
}

void Game::handlePlayerInput(float dt)
{
    const auto& actionMapping = inputManager.getActionMapping();
    // a bit lazy, better store somewhere
    static const auto horizonalWalkAxis = actionMapping.getActionTagHash("MoveX");
    static const auto verticalWalkAxis = actionMapping.getActionTagHash("MoveY");

    const auto moveStickState =
        util::getStickState(actionMapping, horizonalWalkAxis, verticalWalkAxis);

    glm::vec3 moveDir{};
    auto player = entityutil::getPlayerEntity(registry);

    if (moveStickState != glm::vec2{}) {
        const auto heading = util::calculateStickHeading(camera, moveStickState);
        moveDir = heading;

        const auto angle = std::atan2(heading.x, heading.z);
        const auto targetHeading = glm::angleAxis(angle, math::GlobalUpAxis);
        eu::rotateSmoothlyTo(player, targetHeading, 0.12f);
    } else {
        moveDir = {};
        eu::stopRotation(player);
    }

    static const auto sprintAction = actionMapping.getActionTagHash("Sprint");
    static const auto jumpAction = actionMapping.getActionTagHash("Jump");
    bool jumping = actionMapping.wasJustPressed(jumpAction);
    bool jumpHeld = actionMapping.isHeld(jumpAction);
    bool sprinting = actionMapping.isHeld(sprintAction);
    physicsSystem->handleCharacterInput(dt, moveDir, jumping, jumpHeld, sprinting);
}

void Game::updateFollowCamera(entt::const_handle followEntity, float dt)
{
    const auto& tc = followEntity.get<TransformComponent>();

    bool isPlayer = (followEntity == entityutil::getPlayerEntity(registry));
    float zOffset = 0.f;
    if (smoothSmartCamera && isPlayer) {
        zOffset = calculateSmartZOffset(followEntity, dt);
    }

    prevOffsetZ = zOffset;

    float yOffset = cameraYOffset;

    static float timeFalling = 0.f;
    static float timeJumping = 0.f;
    if (isPlayer) {
        if (!physicsSystem->isCharacterOnGround()) {
            if (followEntity.get<MovementComponent>().effectiveVelocity.y < 0.f) {
                timeJumping = 0.f;
                timeFalling += dt;
                yOffset = cameraYOffset - std::min(timeFalling / 0.5f, 1.f) * cameraYOffset;
            } else {
                timeJumping += dt;
            }
        } else {
            timeFalling = 0.f;
            timeJumping = 0.f;
        }
    }
    const auto orbitTarget = getCameraOrbitTarget(followEntity, yOffset, zOffset);

    if (drawCameraTrackPoint) {
        Im3d::PushLayerId(Im3dState::WorldNoDepthLayer);
        Im3d::DrawPoint({orbitTarget.x, orbitTarget.y, orbitTarget.z}, 5.f, Im3d::Color_Green);
        // Im3dText(orbitTarget, 1.f, RGBColor{255, 255, 255}, std::to_string(zOffset).c_str());
        Im3d::PopLayerId();
    }

    if (!smoothCamera) {
        cameraCurrentTrackPointPos = orbitTarget;
        auto targetPos =
            getCameraDesiredPosition(camera, cameraCurrentTrackPointPos, orbitDistance + 2.f);
        camera.setPosition(targetPos);
        return;
    }

    glm::vec3 maxSpeed = cameraMaxSpeed;
    // when the character moves at full speed, rotation can be too much
    // so it's better to slow down the camera to catch up slower
    if (zOffset > 0.5 * maxCameraOffsetFactorRun * cameraZOffset) {
        maxSpeed.x *= 0.75f;
        maxSpeed.z *= 0.75f;
    }

    // to catch up with the falling player
    if (isPlayer && timeFalling > 0.5f) {
        maxSpeed.y *= 2.f;
    }

    cameraCurrentTrackPointPos = util::smoothDamp(
        cameraCurrentTrackPointPos, orbitTarget, followCameraVelocity, cameraDelay, dt, maxSpeed);

    // change orbit distance based on pitch
    float orbitDist = orbitDistance;
    float orbitPitchChange =
        std::clamp((camera.getPosition().y - cameraCurrentTrackPointPos.y) / testParam, -2.f, 2.f);
    orbitDist += orbitPitchChange;

    const auto targetPos = getCameraDesiredPosition(camera, cameraCurrentTrackPointPos, orbitDist);
    camera.setPosition(targetPos);
}

float Game::calculateSmartZOffset(entt::const_handle followEntity, float dt)
{
    bool playerRunning = false;
    { // TODO: make this prettier?
        const auto player = eu::getPlayerEntity(registry);
        const auto& mc = player.get<MovementComponent>();
        auto velocity = mc.effectiveVelocity;
        velocity.y = 0.f;
        const auto velMag = glm::length(velocity);
        playerRunning = velMag > 1.5f;
    }

    float desiredOffset = cameraZOffset;
    if (playerRunning) {
        desiredOffset = cameraZOffset * maxCameraOffsetFactorRun;
    }

    if (prevOffsetZ == desiredOffset) {
        return desiredOffset;
    }

    // this prevents noise - we wait for at least desiredOffsetChangeDelay
    // before changing the desired offset
    if (prevDesiredOffset != desiredOffset) {
        float delay = (desiredOffset == cameraZOffset) ? desiredOffsetChangeDelayRecenter :
                                                         desiredOffsetChangeDelayRun;
        if (desiredOffsetChangeTimer < delay) {
            desiredOffsetChangeTimer += dt;
            return prevOffsetZ;
        } else {
            desiredOffsetChangeTimer = 0.f;
        }
    }
    prevDesiredOffset = desiredOffset;

    // move from max offset to default offset in cameraMaxOffsetTime
    float v = (cameraZOffset * maxCameraOffsetFactorRun - cameraZOffset) / cameraMaxOffsetTime;
    float sign = (prevOffsetZ > desiredOffset) ? -1.f : 1.f;
    if (sign == -1.f) {
        // move faster to default position (cameraZOffset)
        // (otherwise the camera will wait until the player matches with the
        // center which looks weird)
        v *= 1.5f;
    }
    return std::
        clamp(prevOffsetZ + sign * v * dt, cameraZOffset, cameraZOffset * maxCameraOffsetFactorRun);
}

void Game::customCleanup()
{
    registry.clear();

    animationSoundSystem.cleanup(eventManager);

    physicsSystem->cleanup();
    physicsSystem.reset();

    gfxDevice.waitIdle();
    im3d.cleanup(gfxDevice);
    spriteRenderer.cleanup();
    renderer.cleanup();
    materialCache.cleanup(gfxDevice);
    meshCache.cleanup(gfxDevice);
}

void Game::setEntityTag(entt::handle e, const std::string& tag)
{
    auto& tc = e.get_or_emplace<TagComponent>();
    if (!tc.tag.empty()) {
        taggedEntities.erase(tc.tag);
    }

    tc.tag = tag;
    auto [it, inserted] = taggedEntities.emplace(tc.tag, e.entity());
    assert(inserted);
}

entt::handle Game::findEntityByTag(const std::string& tag)
{
    auto it = taggedEntities.find(tag);
    if (it != taggedEntities.end()) {
        return entt::handle{registry, it->second};
    }
    return {};
}

entt::const_handle Game::findEntityByTag(const std::string& tag) const
{
    auto it = taggedEntities.find(tag);
    if (it != taggedEntities.end()) {
        return entt::const_handle{registry, it->second};
    }
    return {};
}

void Game::setCurrentCamera(entt::const_handle cameraEnt)
{
    if (cameraEnt.entity() == entt::null) {
        fmt::println("Passed null into setCurrentCamera");
        return;
    }
    if (auto ncPtr = cameraEnt.try_get<NameComponent>(); ncPtr) {
        fmt::println("Current camera: {}", ncPtr->name);
    } else {
        fmt::println("Current camera: {}", cameraEnt.get<MetaInfoComponent>().sceneNodeName);
    }
    auto& tc = cameraEnt.get<TransformComponent>();
    camera.setPosition(tc.transform.getPosition());
    camera.setHeading(tc.transform.getHeading());
}

entt::const_handle Game::findDefaultCamera()
{
    auto cameraEnt = eu::findCameraByName(registry, "Default");
    if (cameraEnt.entity() != entt::null) {
        return cameraEnt;
    }
    return eu::findEntityBySceneNodeName(registry, "Camera");
}

namespace
{
bool shouldCastShadow(const entt::const_handle e)
{
    const auto& tag = eu::getTag(e);
    return !tag.starts_with("GroundTile");
}
}

void Game::customDraw()
{
    {
        ZoneScopedN("Generate draw list");
        generateDrawList();
    }

    {
        ZoneScopedN("Render");
        const auto sceneData = GameRenderer::SceneData{
            .camera = camera,
            .ambientColor = level.getAmbientLightColor(),
            .ambientIntensity = level.getAmbientLightIntensity(),
            .fogColor = level.getFogColor(),
            .fogDensity = level.getFogDensity(),
        };

        auto cmd = gfxDevice.beginFrame();
        renderer.draw(cmd, camera, sceneData);

        { // UI
            vkutil::cmdBeginLabel(cmd, "UI");
            spriteRenderer.draw(cmd, renderer.getDrawImage());
            vkutil::cmdEndLabel(cmd);
        }

        const auto& drawImage = renderer.getDrawImage();
        { // Im3D
            vkutil::cmdBeginLabel(cmd, "im3d");
            im3d.draw(
                cmd,
                gfxDevice,
                drawImage.imageView,
                drawImage.getExtent2D(),
                renderer.getDepthImage().imageView);
            vkutil::cmdEndLabel(cmd);
        }

        const auto endFrameProps = GfxDevice::EndFrameProps{
            .clearColor = gameDrawnInWindow ? edbr::rgbToLinear(97, 120, 159) :
                                              LinearColor{0.f, 0.f, 0.f, 1.f},
            .copyImageIntoSwapchain = !gameDrawnInWindow,
            .drawImageBlitRect =
                {gameWindowPos.x, gameWindowPos.y, gameWindowSize.x, gameWindowSize.y},
            .drawImGui = drawImGui,
        };

        gfxDevice.endFrame(cmd, drawImage, endFrameProps);
    }
}

void Game::generateDrawList()
{
    renderer.beginDrawing();

    // add lights
    const auto lights = registry.view<TransformComponent, LightComponent>();
    for (const auto&& [e, tc, lc] : lights.each()) {
        renderer.addLight(lc.light, tc.transform);
    }

    // render static meshes
    const auto staticMeshes = registry.view<TransformComponent, MeshComponent>(
        entt::exclude<SkeletonComponent, TriggerComponent, ColliderComponent>);
    for (const auto&& [e, tc, mc] : staticMeshes.each()) {
        bool castShadow = shouldCastShadow({registry, e});
        for (std::size_t i = 0; i < mc.meshes.size(); ++i) {
            const auto meshTransform = mc.meshTransforms[i].isIdentity() ?
                                           tc.worldTransform :
                                           tc.worldTransform * mc.meshTransforms[i].asMatrix();
            renderer.drawMesh(mc.meshes[i], meshTransform, castShadow);
        }
    }

    // render meshes with skeletal animation
    const auto skinnedMeshes =
        registry.view<TransformComponent, MeshComponent, SkeletonComponent>();
    for (const auto&& [e, tc, mc, sc] : skinnedMeshes.each()) {
        renderer.drawSkinnedMesh(
            mc.meshes, sc.skinnedMeshes, tc.worldTransform, sc.skeletonAnimator.getJointMatrices());
#ifndef NDEBUG
        // 1. Not all meshes for the entity might be skinned
        // 2. Different meshes can have different joint matrices sets
        // But right now it's better to group meshes to not waste memory
        // It's also assumed that all skinned meshes have meshTransform[i] == I
        for (const auto& t : mc.meshTransforms) {
            assert(t.isIdentity());
        }
#endif
    }

    renderer.endDrawing();

    spriteRenderer.beginDrawing();
    ui.draw(spriteRenderer);
    spriteRenderer.endDrawing();
}

void Game::loadLevel(const std::filesystem::path& path)
{
    level.load(path);

    // load skybox
    if (level.hasSkybox()) {
        const auto skyboxImageId = gfxDevice.addImageToCache(
            graphics::loadCubemap(gfxDevice, skyboxDir / level.getSkyboxName()));
        renderer.setSkyboxImage(skyboxImageId);
    } else {
        // no skybox
        renderer.setSkyboxImage(NULL_IMAGE_ID);
    }

    if (const auto& bgmPath = level.getBGMPath(); !bgmPath.empty()) {
        audioManager.playMusic(bgmPath);
    }

    loadScene(level.getSceneModelPath(), true);
}

util::SceneLoadContext Game::createSceneLoadContext()
{
    return util::SceneLoadContext{
        .gfxDevice = gfxDevice,
        .meshCache = meshCache,
        .materialCache = materialCache,
        .entityFactory = entityFactory,
        .registry = registry,
        .sceneCache = sceneCache,
        .defaultPrefabName = "static_geometry",
        .physicsSystem = *physicsSystem,
        .animationCache = animationCache,
    };
}

void Game::loadScene(const std::filesystem::path& path, bool createEntities)
{
    const auto& scene = sceneCache.loadScene(gfxDevice, meshCache, materialCache, path);
    if (createEntities) {
        util::createEntitiesFromScene(createSceneLoadContext(), scene);
    }
}

void Game::onTagComponentDestroy(entt::registry& registry, entt::entity entity)
{
    auto& tc = registry.get<TagComponent>(entity);
    taggedEntities.erase(tc.tag);
}

void Game::onSkeletonComponentDestroy(entt::registry& registry, entt::entity entity)
{
    auto& sc = registry.get<SkeletonComponent>(entity);
    for (const auto& skinnedMesh : sc.skinnedMeshes) {
        renderer.getGfxDevice().destroyBuffer(skinnedMesh.skinnedVertexBuffer);
    }
}

void Game::initEntityFactory()
{
    entityFactory.setCreateDefaultEntityFunc([](entt::registry& registry) {
        auto e = registry.create();
        registry.emplace<TransformComponent>(e);
        registry.emplace<HierarchyComponent>(e);
        return entt::handle(registry, e);
    });

    entityFactory.setPostInitEntityFunc(
        [this](entt::handle e) { entityInitializer.initEntity(e); });

    const auto prefabsDir = std::filesystem::path{"assets/prefabs"};
    loadPrefabs(prefabsDir);

    entityFactory.addMappedPrefabName("interact", "trigger");
    entityFactory.addMappedPrefabName("guardrail", "static_geometry_no_coll");
    entityFactory.addMappedPrefabName("stairs", "static_geometry_no_coll");
    entityFactory.addMappedPrefabName("railing", "static_geometry_no_coll");
}

void Game::loadPrefabs(const std::filesystem::path& prefabsDir)
{
    // Automatically load all prefabs from the directory
    // Prefab from "assets/prefabs/npc/guy.json" is named "npc/guy"
    util::foreachFileInDir(prefabsDir, [this, &prefabsDir](const std::filesystem::path& p) {
        auto relPath = p.lexically_relative(prefabsDir);
        const auto prefabName = relPath.replace_extension("").string();
        entityFactory.registerPrefab(prefabName, p);
    });
}

void Game::registerComponents(ComponentFactory& componentFactory)
{
    registry.on_destroy<TagComponent>().connect<&Game::onTagComponentDestroy>(this);
    registry.on_destroy<SkeletonComponent>().connect<&Game::onSkeletonComponentDestroy>(this);

    componentFactory.registerComponent<TriggerComponent>("trigger");
    componentFactory.registerComponent<CameraComponent>("camera");
    componentFactory.registerComponent<PlayerSpawnComponent>("player_spawn");
    componentFactory.registerComponent<ColliderComponent>("collider");

    componentFactory.registerComponentLoader(
        "mesh", [](entt::handle e, MeshComponent& mc, const JsonDataLoader& loader) {
            loader.get("mesh", mc.meshPath);
        });

    componentFactory.registerComponentLoader(
        "movement", [](entt::handle e, MovementComponent& mc, const JsonDataLoader& loader) {
            loader.get("maxSpeed", mc.maxSpeed);
        });

    componentFactory.registerComponentLoader(
        "animation_event_sound",
        [](entt::handle e, AnimationEventSoundComponent& sc, const JsonDataLoader& loader) {
            sc.eventSounds = loader.getLoader("events").getKeyValueMapString();
        });
}