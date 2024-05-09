#include "Game.h"

#include "Components.h"
#include "EntityUtil.h"
#include "FollowCameraController.h"
#include "MTPSaveFile.h"
#include "Systems.h"

#include <edbr/ActionList/ActionWrappers.h>
#include <edbr/Camera/FreeCameraController.h>

#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/ECS/Components/PersistentComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>

#include <edbr/ECS/Systems/MovementSystem.h>
#include <edbr/ECS/Systems/TransformSystem.h>

#include <edbr/GameCommon/DialogueActions.h>
#include <edbr/Graphics/CoordUtil.h>
#include <edbr/Graphics/Cubemap.h>
#include <edbr/Graphics/Letterbox.h>
#include <edbr/Graphics/Scene.h>
#include <edbr/Graphics/Vulkan/Util.h>
#include <edbr/Math/Util.h>
#include <edbr/Util/CameraUtil.h>
#include <edbr/Util/FS.h>
#include <edbr/Util/InputUtil.h>

#include <tracy/Tracy.hpp>

#include <glm/gtx/norm.hpp> // distance2

namespace eu = entityutil;

Game::Game() :
    Application(),
    renderer(gfxDevice, meshCache, materialCache),
    spriteRenderer(gfxDevice),
    sceneCache(gfxDevice, meshCache, materialCache, animationCache),
    entityCreator(registry, "static_geometry", entityFactory, sceneCache),
    animationSoundSystem(audioManager),
    cameraManager(actionListManager),
    ui(actionListManager, audioManager)
{}

void Game::defineCLIArgs()
{
    Application::defineCLIArgs();
    cliApp.add_option("-l,--level", devLevelToLoad, "Level to load (dev only)");
    cliApp.add_option("--spawn", devStartSpawnPoint, "Spawn to start from (dev only)");
}

void Game::loadAppSettings()
{
    const std::filesystem::path appSettingsPath{"assets/data/default_app_settings.json"};
    JsonFile file(appSettingsPath);
    if (!file.isGood()) {
        fmt::println("failed to load dev settings from {}", appSettingsPath.string());
        return;
    }

    const auto loader = file.getLoader();
    loader.getIfExists("renderResolution", params.renderSize);
    loader.getIfExists("vSync", vSync);

    params.version = Version{
        .major = 0,
        .minor = 1,
        .patch = 1,
    };
}

void Game::customInit()
{
    inputManager.getActionMapping().loadActions("assets/data/input_actions.json");
    inputManager.loadMapping("assets/data/input_mapping.json");
    textManager.loadText("en", "assets/text/en.json", "");
    handleSaveFiles();

    animationCache.loadAnimationData("assets/data/animation_data.json");

    skyboxDir = "assets/images/skybox";

    materialCache.init(gfxDevice);
    renderer.init(params.renderSize);
    spriteRenderer.init(renderer.getDrawImageFormat());

    animationSoundSystem.init(eventManager, "assets/sounds");

    im3d.init(gfxDevice, renderer.getDrawImageFormat(), renderer.getDepthImageFormat());

    // register entity stuff
    initEntityFactory();
    registerComponents(entityFactory.getComponentFactory());
    registerComponentDisplayers();
    entityCreator.setPostInitEntityFunc([this](entt::handle e) { entityPostInit(e); });
    eu::setEventManager(eventManager);

    { // physics
        PhysicsSystem::InitStaticObjects();
        physicsSystem = std::make_unique<PhysicsSystem>(eventManager);
        physicsSystem->init();

        // sub to physics events
        eventManager.addListener(this, &Game::onCollisionStarted);
        eventManager.addListener(this, &Game::onCollisionEnded);
    }

    // create UI
    ui.init(*this, gfxDevice, params.renderSize);

    { // create camera
        static const float aspectRatio = (float)params.renderSize.x / (float)params.renderSize.y;

        camera.setUseInverseDepth(true);
        camera.init(cameraFovX, cameraNear, cameraFar, aspectRatio);
    }

    { // create camera controllers
        cameraManager
            .addController(freeCameraControllerTag, std::make_unique<FreeCameraController>());
        cameraManager.addController(
            followCameraControllerTag, std::make_unique<FollowCameraController>(*physicsSystem));
    }

    registerLevels();

    if (!isDevEnvironment) {
        enterMainMenu();
    } else {
        audioManager.setMuted(true);
        if (!saveFileManager.saveFileExists(0)) {
            startNewGame();
        } else {
            startLoadedGame();
        }
    }
}

void Game::customCleanup()
{
    for (auto entity : registry.view<entt::entity>()) {
        if (!registry.get<HierarchyComponent>(entity).hasParent()) {
            destroyEntity({registry, entity}, false);
        }
    }
    registry.clear();

    // unsub from physics events
    eventManager.removeListener<CharacterCollisionStartedEvent>(this);
    eventManager.removeListener<CharacterCollisionEndedEvent>(this);

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

void Game::startNewGame()
{
    saveFileManager.setCurrentSaveIndex(0);

    const auto newGameSaveFilePath = std::filesystem::path{"assets/data/new_game_save_file.json"};
    saveFileManager.getCurrentSaveFile().loadFromFile(newGameSaveFilePath);
    fmt::println("No save files found: creating one from {}", newGameSaveFilePath.string());

    startGameplay();
}

void Game::startLoadedGame()
{
    // TODO "load game" screen
    assert(saveFileManager.saveFileExists(0));
    saveFileManager.setCurrentSaveIndex(0);
    if (saveFileManager.saveFileExists(0)) {
        saveFileManager.loadCurrentSaveFile();
    }

    startGameplay();
}

void Game::enterMainMenu()
{
    gameState = GameState::MainMenu;
    doWithDelay("show main menu", 0.5f, [this]() { ui.enterMainMenu(); });
    changeLevel("main_menu");
}

void Game::exitGame()
{
    isRunning = false;
}

void Game::startGameplay()
{
    playerInputEnabled = false;

    auto l = ActionList(
        "exit_main_menu", //
        exitLevel(LevelTransitionType::Teleport), // exit from main menu level
        [this]() {
            // so that the game doesn't think we have a previous level
            level = Level{};

            assert(!eu::playerExists(registry));
            { // create player
                static const std::string playerPrefabName = "cato";
                auto e = entityCreator.createFromPrefab(playerPrefabName);
                e.emplace<PlayerComponent>();
                eu::makePersistent(e);
            }

            const auto& saveFile = getSaveFile();
            std::string startLevel = saveFile.getLevelName();
            std::string startSpawnPoint = saveFile.getSpawnName();

            // dev override
            if (isDevEnvironment && !devLevelToLoad.empty()) {
                startLevel = devLevelToLoad;
                startSpawnPoint = devStartSpawnPoint;

                // so that new levels loaded as usual
                devLevelToLoad.clear();
            }

            changeLevel(startLevel, startSpawnPoint);
            doLevelChange(); // immediately do level change here
            gameState = GameState::Playing;
        });

    actionListManager.addActionList(std::move(l));

    actionListManager.addActionList(ActionList{
        "test dialogue box",
        actions::delay(1.f),
        say(dialogue::TextToken{
            .text = LST{"GAME_SAVED"},
            .name = LST{"DUDE_NAME"},
        })});
}

void Game::enterPauseMenu()
{
    assert(gameState == GameState::Playing);
    gameState = GameState::Paused;
    ui.enterPauseMenu();
}

void Game::exitPauseMenu()
{
    assert(gameState == GameState::Paused);
    gameState = GameState::Playing;
    ui.closePauseMenu();
}

void Game::exitToTitle()
{
    assert(gameState == GameState::Paused);

    ui.closePauseMenu();
    cameraManager.stopCameraTransitions();

    // TODO: stop ALL music and sounds
    if (const auto& bgmPath = level.getBGMPath(); !bgmPath.empty()) {
        audioManager.stopMusic(bgmPath.string());
    }

    // destory level
    renderer.setSkyboxImage(NULL_IMAGE_ID);
    level = Level{};

    // just destroy all entities?
    eu::makeNonPersistent(eu::getPlayerEntity(registry));
    destroyNonPersistentEntities();

    enterMainMenu();
}

void Game::handleSaveFiles()
{
    // TODO: use saveFileManager.findSaveFileDir when it's implemented
    const auto saveFileDir = std::filesystem::path{"saves"};
    if (!std::filesystem::exists(saveFileDir)) {
        std::filesystem::create_directory(saveFileDir);
    }
    saveFileManager.registerSaveFiles<MTPSaveFile>(NumSaveFiles, saveFileDir, *this);
}

bool Game::hasAnySaveFile() const
{
    for (std::size_t i = 0; i < NumSaveFiles; ++i) {
        if (saveFileManager.saveFileExists(i)) {
            return true;
        }
    }
    return false;
}

void Game::customUpdate(float dt)
{
    ZoneScopedN("Update");

    if (isDevEnvironment && devResumeForOneFrame) {
        devResumeForOneFrame = false;
        devPaused = true;
    }

    if (!gameDrawnInWindow) {
        const auto blitRect = util::
            calculateLetterbox(renderer.getDrawImage().getSize2D(), gfxDevice.getSwapchainSize());
        gameWindowPos = {blitRect.x, blitRect.y};
        gameWindowSize = {blitRect.z, blitRect.w};
    }

    if (isDevEnvironment) {
        devToolsNewFrame(dt);
    }

    // changeLevel loads level with a delay so that the call to it
    // in the middle of the frame doesn't lead to unexpected results
    if (!newLevelToLoad.empty()) {
        doLevelChange();
    }

    // input
    auto& io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard && !io.WantTextInput) {
        handleInput(dt);
    }

    if (!gamePaused) {
        updateGameLogic(dt);
    }

    if (isDevEnvironment && devPaused) {
        // allow free camera movement inside dev pause
        if (cameraManager.getCurrentControllerTag() == freeCameraControllerTag) {
            cameraManager.update(camera, dt);
        }
    }

    ui.update(dt);

    if (isDevEnvironment) {
        devToolsUpdate(dt);
    }
}

glm::ivec2 Game::getGameScreenSize() const
{
    return params.renderSize;
}

glm::vec2 Game::getMouseGameScreenCoord() const
{
    const auto& mouse = inputManager.getMouse();
    return edbr::util::getGameWindowScreenCoord(
        mouse.getPosition(), gameWindowPos, gameWindowSize, getGameScreenSize());
}

void Game::updateGameLogic(float dt)
{
    // movement
    edbr::ecs::movementSystemUpdate(registry, dt);

    { // physics
        // get player heading (if player exist)
        const auto player = entityutil::getPlayerEntity(registry);
        glm::quat playerHeading = {};
        if (player.entity() != entt::null) {
            playerHeading = player.get<TransformComponent>().transform.getHeading();
        }

        physicsSystem->update(dt, playerHeading);

        // sync visible transforms
        physicsSystem->syncCharacterTransform();
        auto physicsView = registry.view<TransformComponent, PhysicsComponent>();
        for (auto&& [e, tc, pc] : physicsView.each()) {
            physicsSystem->syncVisibleTransform(pc.bodyId, tc.transform);
        }

        if (player.entity() != entt::null) { // find closest interactable entity
            interactEntity = {};
            float minDist = std::numeric_limits<float>::max();
            for (const auto& e : physicsSystem->getInteractableEntities()) {
                float d =
                    glm::distance2(eu::getWorldPosition(e), physicsSystem->getCharacterPosition());
                if (d < minDist) {
                    interactEntity = e;
                }
            }
        }
    }

    edbr::ecs::transformSystemUpdate(registry, dt);
    edbr::ecs::movementSystemPostPhysicsUpdate(registry, dt);
    if (auto player = entityutil::getPlayerEntity(registry); player.entity() != entt::null) {
        playerAnimationSystemUpdate(player, *physicsSystem, dt);
    }
    skeletonAnimationSystemUpdate(registry, eventManager, dt);

    // camera update
    cameraManager.update(camera, dt);

    // audio needs to be updated after the camera to set listener's position
    animationSoundSystem.update(registry, camera, dt);

    // level script update
    getLevelScript().update(dt);
}

void Game::handleInput(float dt)
{
    const auto& am = inputManager.getActionMapping();
    static const auto pauseAction = am.getActionTagHash("Pause");
    static const auto menuBackAction = am.getActionTagHash("MenuBack");

    cameraManager.handleInput(inputManager, camera, dt);

    if (playerInputEnabled) {
        if (ui.capturesInput()) {
            ui.handleInput(inputManager, dt);

        } else {
            // HACK: player input will get handled in the next frame after
            // the menu closes - ideally this should be handled
            // by setting some timer which will ignore interactions
            // for some amount of time
            if (entityutil::playerExists(registry)) {
                handlePlayerInput(dt);
            }
        }

        // pause menu inputs
        if (gameState == GameState::Playing && !ui.capturesInput()) {
            // pause menu enter
            if (am.wasJustPressed(pauseAction)) {
                enterPauseMenu();
            }
        } else if (gameState == GameState::Paused) {
            // pause menu exit
            const auto& am = inputManager.getActionMapping();
            // can exit pause menu by pressing "Pause" and "MenuBack"
            if (am.wasJustPressed(pauseAction) || am.wasJustPressed(menuBackAction)) {
                exitPauseMenu();
            }
        }
    }

    if (isDevEnvironment) {
        devToolsHandleInput(dt);
    }

    gamePaused = ui.isInPauseMenu();
    gamePaused = gamePaused || devPaused;
}

void Game::handlePlayerInput(float dt)
{
    const auto& actionMapping = inputManager.getActionMapping();
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

    // handle movement
    static const auto sprintAction = actionMapping.getActionTagHash("Sprint");
    static const auto jumpAction = actionMapping.getActionTagHash("Jump");
    bool jumping = actionMapping.wasJustPressed(jumpAction);
    bool jumpHeld = actionMapping.isHeld(jumpAction);
    bool sprinting = actionMapping.isHeld(sprintAction);
    physicsSystem->handleCharacterInput(dt, moveDir, jumping, jumpHeld, sprinting);

    // handle interaction
    static const auto interactAction = actionMapping.getActionTagHash("PrimaryAction");
    if (actionMapping.wasJustPressed(interactAction) && interactEntity.entity() != entt::null) {
        handlePlayerInteraction();
    }
}

void Game::handlePlayerInteraction()
{
    const auto& interactEntityName = eu::getMetaName(interactEntity);
    auto res = getLevelScript().onEntityInteract(interactEntity, interactEntityName);

    if (std::holds_alternative<LocalizedStringTag>(res)) {
        const auto& t = std::get<LocalizedStringTag>(res);
        actionListManager.addActionList(say(t, interactEntity));
    } else if (std::holds_alternative<std::vector<dialogue::TextToken>>(res)) {
        const auto& tokens = std::get<std::vector<dialogue::TextToken>>(res);
        actionListManager.addActionList(say(tokens, interactEntity));
    } else if (std::holds_alternative<ActionList>(res)) {
        actionListManager.addActionList(std::move(std::get<ActionList>(res)));
    }
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

void Game::setCurrentCamera(entt::const_handle cameraEnt, float transitionTime)
{
    if (cameraEnt.entity() == entt::null) {
        fmt::println("Passed null into setCurrentCamera");
        return;
    }
    // fmt::println("Current camera: {}", eu::getMetaName(cameraEnt));

    const auto& tc = cameraEnt.get<TransformComponent>();
    cameraManager.setCamera(tc.transform, camera, transitionTime);
}

void Game::setCurrentCamera(const std::string& cameraTag, float transitionTime)
{
    setCurrentCamera(eu::findCameraByName(registry, cameraTag), transitionTime);
}

void Game::setFollowCamera(float transitionTime)
{
    cameraManager.setController(followCameraControllerTag, camera, transitionTime);
}

FollowCameraController& Game::getFollowCameraController()
{
    return static_cast<FollowCameraController&>(
        cameraManager.getController(followCameraControllerTag));
}

void Game::cameraFollowEntity(entt::handle e, bool instantTeleport)
{
    auto& fcc = getFollowCameraController();
    fcc.init(camera);
    fcc.startFollowingEntity(e, camera, instantTeleport);
    cameraManager.setController(followCameraControllerTag);
}

entt::const_handle Game::findDefaultCamera()
{
    auto cameraEnt = eu::findCameraByName(registry, "Default");
    if (cameraEnt.entity() != entt::null) {
        return cameraEnt;
    }
    // return first camera we can find
    for (const auto& e : registry.view<CameraComponent>()) {
        return {registry, e};
    }
    return {};
}

void Game::customDraw()
{
    {
        ZoneScopedN("Generate draw list");
        generateDrawList();
    }

    {
        ZoneScopedN("Render");
        auto sceneData = GameRenderer::SceneData{
            .camera = camera,
            .ambientColor = level.getAmbientLightColor(),
            .ambientIntensity = level.getAmbientLightIntensity(),
        };
        if (level.isFogActive()) {
            sceneData.fogColor = level.getFogColor();
            sceneData.fogDensity = level.getFogDensity();
        }

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

        if (!isDevEnvironment) {
            gameDrawnInWindow = false;
            drawImGui = false;
        }

        const auto devClearBgColor = edbr::rgbToLinear(97, 120, 159);
        const auto endFrameProps = GfxDevice::EndFrameProps{
            .clearColor = gameDrawnInWindow ? devClearBgColor : LinearColor::Black(),
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
        entt::
            exclude<SkeletonComponent, TriggerComponent, ColliderComponent, PlayerSpawnComponent>);
    for (const auto&& [e, tc, mc] : staticMeshes.each()) {
        for (std::size_t i = 0; i < mc.meshes.size(); ++i) {
            const auto meshTransform = mc.meshTransforms[i].isIdentity() ?
                                           tc.worldTransform :
                                           tc.worldTransform * mc.meshTransforms[i].asMatrix();
            renderer.drawMesh(mc.meshes[i], meshTransform, mc.castShadow);
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

    // UI
    {
        spriteRenderer.beginDrawing();
        auto uiCtx = GameUI::UIContext{
            .camera = camera,
            .screenSize = params.renderSize,
        };
        if (eu::playerExists(registry)) {
            uiCtx.playerPos = physicsSystem->getCharacterPosition();
            if (interactEntity.entity() != entt::null) {
                const auto& ic = interactEntity.get<InteractComponent>();
                uiCtx.interactionType = ic.type;
            }
        }
        ui.draw(spriteRenderer, uiCtx);
        devToolsDrawUI(spriteRenderer);
        spriteRenderer.endDrawing();
    }
}

void Game::loadLevel(const std::filesystem::path& path)
{
    bool loadedFromModel{false};

    if (std::filesystem::exists(path)) {
        level.load(path);
    } else if (isDevEnvironment) {
        // special level loading mode for dev environment:
        // load from assets/models/levels/<level>/<level>.gltf
        // Player will be destroyed and free camera will be enabled
        // by default
        auto levelName = path.filename().replace_extension("");
        const auto modelPath = "assets/models/levels" / levelName / (levelName.string() + ".gltf");
        fmt::println(
            "level {} was not found, trying to load model from {}",
            path.string(),
            modelPath.string());
        level.loadFromModel(modelPath);
        loadedFromModel = true;
    }
    level.setName(path.filename().replace_extension("").string());

    // load skybox
    if (level.hasSkybox()) {
        const auto skyboxImageId = gfxDevice.addImageToCache(
            graphics::loadCubemap(gfxDevice, skyboxDir / level.getSkyboxName()));
        renderer.setSkyboxImage(skyboxImageId);
    } else {
        // no skybox
        renderer.setSkyboxImage(NULL_IMAGE_ID);
    }

    // spawn entities
    const auto& scene = sceneCache.loadOrGetScene(level.getSceneModelPath());
    auto createdEntities = entityCreator.createEntitiesFromScene(scene);
    (void)createdEntities; // maybe will do something with them later...

    // this will update worldTransforms to actual state
    edbr::ecs::transformSystemUpdate(registry, 0.f);

    if (loadedFromModel) {
        destroyEntity(eu::getPlayerEntity(registry));
        setCurrentCamera(findDefaultCamera());
        freeCameraMode = true;
        playerInputEnabled = false;
        cameraManager.setController(freeCameraControllerTag);
    }
}

void Game::changeLevel(
    const std::string& levelTag,
    const std::string& spawnName,
    LevelTransitionType ltt)
{
    if (actionListManager.isActionListPlaying(levelTransitionListName)) {
        return;
    }

    // TODO: check that level exists to print error immediately
    newLevelToLoad = "assets/levels/" + levelTag + ".json";
    // if spawnName is empty, the default spawn on the level will be used
    newLevelSpawnName = spawnName;
    newLevelTransitionType = ltt;
}

void Game::doLevelChange()
{
    using namespace actions;
    const auto levelToLoad = newLevelToLoad;
    const auto spawnName = newLevelSpawnName;

    auto levelTransition = ActionList(levelTransitionListName);

    bool hasPreviousLevel = !level.getName().empty();
    if (hasPreviousLevel) {
        levelTransition.addAction(exitLevel(newLevelTransitionType));
    }

    levelTransition.addActions(
        doNamed(
            "Spawn player",
            [this, levelToLoad, spawnName] {
                loadLevel(levelToLoad);
                // spawn player
                const auto& spawnPoint =
                    spawnName.empty() ? level.getDefaultPlayerSpawnerName() : spawnName;
                lastSpawnName = spawnPoint;
                eu::spawnPlayer(registry, spawnPoint);
            }),
        enterLevel(newLevelTransitionType) // enter
    );

    // play action list
    actionListManager.addActionList(std::move(levelTransition));

    newLevelToLoad.clear();
    newLevelSpawnName.clear();
}

ActionList Game::enterLevel(LevelTransitionType ltt)
{
    using namespace actions;
    return ActionList(
        "Enter level",
        doNamed(
            "Enter level",
            [this]() {
                if (newLevelTransitionType == LevelTransitionType::EnterDoor) {
                    audioManager.playSound("assets/sounds/door_close.wav");
                }

                // play music
                if (const auto& bgmPath = level.getBGMPath(); !bgmPath.empty()) {
                    audioManager.playMusic(bgmPath.string());
                }

                // handle camera
                const auto& cameraName = level.getDefaultCameraName();
                if (!cameraName.empty()) {
                    setCurrentCamera(eu::findCameraByName(registry, cameraName));
                } else if (eu::playerExists(registry)) {
                    cameraFollowEntity(eu::getPlayerEntity(registry), true);
                }

                getLevelScript().onLevelEnter();
            }),
        ui.fadeInFromBlackAction(0.5f), //
        doNamed("Enable player input", [this]() {
            // this doesn't feel very good, but prevents player from messing things up
            if (eu::playerExists(registry)) {
                playerInputEnabled = true;
            }
        }));
}

ActionList Game::exitLevel(LevelTransitionType ltt)
{
    using namespace actions;
    return ActionList(
        "Exit level", //
        doNamed(
            "Exit level",
            [this, ltt]() {
                playerInputEnabled = false;
                if (ltt == LevelTransitionType::EnterDoor) {
                    audioManager.playSound("assets/sounds/door_open.wav");
                }

                if (const auto& bgmPath = level.getBGMPath(); !bgmPath.empty()) {
                    audioManager.stopMusic(bgmPath.string());
                }

                getLevelScript().onLevelExit();
            }),
        ui.fadeOutToBlackAction(0.5f),
        doNamed("Stop camera transitions", [this]() { cameraManager.stopCameraTransitions(); }),
        delay(0.2f),
        doNamed("Destroy entities", [this]() { destroyNonPersistentEntities(); }));
}

void Game::initEntityFactory()
{
    entityFactory.setCreateDefaultEntityFunc([](entt::registry& registry) {
        auto e = registry.create();
        registry.emplace<TransformComponent>(e);
        registry.emplace<HierarchyComponent>(e);
        return entt::handle(registry, e);
    });

    const auto prefabsDir = std::filesystem::path{"assets/prefabs"};
    loadPrefabs(prefabsDir);

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

void Game::destroyNonPersistentEntities()
{
    // TODO: move somewhere else?
    interactEntity = {registry, entt::null};

    // Note that this will remove persistent entities if they're not parented
    // to persistent parents!
    for (auto entity : registry.view<entt::entity>()) {
        auto e = entt::handle{registry, entity};
        if (!e.get<HierarchyComponent>().hasParent()) {
            if (!e.all_of<PersistentComponent>()) {
                destroyEntity({registry, entity}, true);
            }
        }
    }
}

void Game::destroyEntity(entt::handle e, bool removeFromRegistry)
{
    auto& hc = e.get<HierarchyComponent>();
    if (hc.hasParent()) {
        // remove from parent's children list
        auto& parentHC = hc.parent.get<HierarchyComponent>();
        std::erase(parentHC.children, e);
    }
    // destory children before removing itself
    for (const auto& child : hc.children) {
        destroyEntity(child, removeFromRegistry);
    }

    if (auto tcPtr = e.try_get<TagComponent>(); tcPtr) {
        if (!tcPtr->getTag().empty()) {
            taggedEntities.erase(tcPtr->getTag());
        }
    }

    if (auto scPtr = e.try_get<SkeletonComponent>(); scPtr) {
        for (const auto& skinnedMesh : scPtr->skinnedMeshes) {
            renderer.getGfxDevice().destroyBuffer(skinnedMesh.skinnedVertexBuffer);
        }
    }

    if (e.all_of<PhysicsComponent>()) {
        physicsSystem->onEntityDestroyed(e);
    }

    if (removeFromRegistry) {
        e.destroy();
    }
}

void Game::onCollisionStarted(const CharacterCollisionStartedEvent& event)
{
    if (event.entity.all_of<TriggerComponent>()) {
        const auto& name = event.entity.get<NameComponent>().name;
        getLevelScript().onTriggerEnter(event.entity, name);
    }
}

void Game::onCollisionEnded(const CharacterCollisionEndedEvent& event)
{
    if (event.entity.all_of<TriggerComponent>()) {
        const auto& name = event.entity.get<NameComponent>().name;
        getLevelScript().onTriggerExit(event.entity, name);
    }
}

MTPSaveFile& Game::getSaveFile()
{
    return static_cast<MTPSaveFile&>(saveFileManager.getCurrentSaveFile());
}

const std::string& Game::getCurrentLevelName() const
{
    return level.getName();
}

const std::string& Game::getLastSpawnName() const
{
    return lastSpawnName;
}

void Game::stopPlayerMovement()
{
    auto player = eu::getPlayerEntity(registry);
    eu::stopRotation(player);
    physicsSystem->stopCharacterMovement();
}

void Game::writeSaveFile()
{
    saveFileManager.writeCurrentSaveFile();
}

LevelScript& Game::getLevelScript()
{
    auto it = levelScripts.find(level.getName());
    if (it != levelScripts.end()) {
        return *it->second;
    }
    static LevelScript emptyLevelScript{*this};
    return emptyLevelScript;
}

namespace
{
LocalizedStringTag getSpeakerName(entt::handle e)
{
    if (auto npcc = e.try_get<NPCComponent>(); npcc) {
        if (!npcc->name.empty()) {
            return npcc->name;
        }
    }
    return LST{};
}
}

ActionList Game::say(const dialogue::TextToken& textToken, entt::handle speaker)
{
    // speaker name
    auto speakerName = textToken.name; // has higher precendence than speaker's name
    if (speakerName.empty() && speaker.entity() != entt::null) {
        speakerName = getSpeakerName(speaker);
    }

    return actions::say(
        textManager,
        ui.getDialogueBox(),
        [this]() { ui.openDialogueBox(); },
        [this]() { ui.closeDialogueBox(); },
        textToken,
        speakerName);
}

ActionList Game::say(const std::vector<dialogue::TextToken>& textTokens, entt::handle speaker)
{
    auto l = ActionList("interaction");
    for (const auto& token : textTokens) {
        l.addAction(say(token, speaker));
    }
    return l;
}

ActionList Game::say(const LocalizedStringTag& text, entt::handle speaker)
{
    const auto textToken = dialogue::TextToken{
        .text = text,
    };
    return say(textToken, speaker);
}

[[nodiscard]] ActionList Game::saveGameSequence()
{
    std::vector<dialogue::TextToken> tokens{
        {
            .text = LST{"DO_YOU_WANT_TO_SAVE"},
            .choices = {LST{"YES_CHOICE"}, LST{"NO_CHOICE"}},
            .onChoice =
                [this](std::size_t index) {
                    if (index == 0) {
                        writeSaveFile();
                        return say(LST{"GAME_SAVED"});
                    }
                    return actions::doNothingList();
                },
        },
    };
    return say(tokens);
}

void Game::playSound(const std::string& soundName)
{
    audioManager.playSound(soundName);
}

void Game::doWithDelay(const std::string& taskName, float delay, std::function<void()> task)
{
    assert(!actionListManager.isActionListPlaying(taskName));
    actionListManager.addActionList(ActionList(taskName, actions::delay(delay), task));
}
