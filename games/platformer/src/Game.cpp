#include "Game.h"

#include <edbr/Core/JsonFile.h>

#include <edbr/GameCommon/DialogueActions.h>
#include <edbr/Graphics/CoordUtil.h>
#include <edbr/Graphics/Letterbox.h>
#include <edbr/Graphics/Vulkan/Util.h>
#include <edbr/Text/LocalizedStringTag.h>
#include <edbr/Util/FS.h>
#include <edbr/Util/InputUtil.h>

#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/ECS/Components/MovementComponent.h>
#include <edbr/ECS/Components/PersistentComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>
#include <edbr/ECS/Systems/MovementSystem.h>
#include <edbr/ECS/Systems/TransformSystem.h>

#include "Components.h"
#include "EntityUtil.h"
#include "Systems.h"

Game::Game() : spriteRenderer(gfxDevice), uiRenderer(gfxDevice), ui(audioManager)
{}

void Game::customInit()
{
    textManager.loadText("en", "assets/text/en.json", "assets/text/en.json");

    inputManager.getActionMapping().loadActions("assets/data/input_actions.json");
    inputManager.loadMapping("assets/data/input_mapping.json");

    drawImageId = gfxDevice.createDrawImage(drawImageFormat, params.renderSize, "draw image");
    spriteRenderer.init(drawImageFormat);
    uiRenderer.init(drawImageFormat);

    devToolsFont.load(gfxDevice, "assets/fonts/at01.ttf", 16, false);

    ui.init(gfxDevice);
    uiInspector.setInspectedUI(ui.getDialogueBox().getRootElement());

    loadAnimations("assets/animations");
    initEntityFactory();
    registerComponents(entityFactory.getComponentFactory());
    registerComponentDisplayers();

    gameCamera.initOrtho2D(static_cast<glm::vec2>(params.renderSize));

    { // create player
        auto player = createEntityFromPrefab("player");
        player.emplace<PlayerComponent>();

        const auto playerPos = glm::vec2{640, 192};
        entityutil::setWorldPosition2D(player, playerPos);
        entityutil::makePersistent(player);
    }

    changeLevel("LevelTown", "from_level_b");
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
        .patch = 0,
    };
}

void Game::initEntityFactory()
{
    entityFactory.setCreateDefaultEntityFunc([](entt::registry& registry) {
        auto e = registry.create();
        registry.emplace<TransformComponent>(e);
        registry.emplace<HierarchyComponent>(e);
        return entt::handle(registry, e);
    });
    entityFactory.setPostInitEntityFunc([this](entt::handle e) { entityPostInit(e); });

    const auto prefabsDir = std::filesystem::path{"assets/prefabs"};
    loadPrefabs(prefabsDir);
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

void Game::loadAnimations(const std::filesystem::path& animationsDir)
{
    // Automatically load all animations from the directory
    // Animations from "assets/animations/npc/guy.json" is named "npc/guy"
    util::foreachFileInDir(animationsDir, [this, &animationsDir](const std::filesystem::path& p) {
        auto relPath = p.lexically_relative(animationsDir);
        const auto animsTag = relPath.replace_extension("").string();
        animationsData[animsTag].load(p);
    });
}

void Game::customCleanup()
{
    entityutil::makeNonPersistent(entityutil::getPlayerEntity(registry));
    destroyNonPersistentEntities();

    gfxDevice.waitIdle();
    spriteRenderer.cleanup();
    uiRenderer.cleanup();
}

void Game::customUpdate(float dt)
{
    if (!newLevelToLoad.empty()) {
        doLevelChange();
    }

    if (!gameDrawnInWindow) {
        const auto blitRect =
            util::calculateLetterbox(params.renderSize, gfxDevice.getSwapchainSize(), true);
        gameWindowPos = {blitRect.x, blitRect.y};
        gameWindowSize = {blitRect.z, blitRect.w};
    }

    handleInput(dt);

    const auto& tileMap = level.getTileMap();

    characterControlSystemUpdate(registry, dt, tileMap);
    edbr::ecs::movementSystemUpdate(registry, dt);
    edbr::ecs::transformSystemUpdate(registry, dt);
    tileCollisionSystemUpdate(registry, dt, tileMap);
    edbr::ecs::movementSystemPostPhysicsUpdate(registry, dt);
    directionSystemUpdate(registry, dt);
    playerAnimationSystemUpdate(registry, dt, tileMap);
    spriteAnimationSystemUpdate(registry, dt);

    // update camera
    if (!freeCamera) {
        auto player = entityutil::getPlayerEntity(registry);
        const auto cameraOffset = glm::vec2{0, -32.f};
        auto cameraPos = entityutil::getWorldPosition2D(player) -
                         static_cast<glm::vec2>(params.renderSize) / 2.f + cameraOffset;
        cameraPos = glm::round(cameraPos);
        gameCamera.setPosition2D(cameraPos);
    }

    if (isDevEnvironment) {
        devToolsUpdate(dt);
    }

    ui.update(static_cast<glm::vec2>(params.renderSize), dt);
}

void Game::handleInput(float dt)
{
    if (playerInputEnabled) {
        const auto& am = inputManager.getActionMapping();
        if (ui.capturesInput()) {
            ui.handleInput(am);
        } else {
            handlePlayerInput(am, dt);
        }
    }

    if (isDevEnvironment) {
        devToolsHandleInput(dt);
    }
}

void Game::handlePlayerInput(const ActionMapping& am, float dt)
{
    static const auto horizonalWalkAxis = am.getActionTagHash("MoveX");
    static const auto verticalWalkAxis = am.getActionTagHash("MoveY");
    const auto moveStickState = util::getStickState(am, horizonalWalkAxis, verticalWalkAxis);

    auto player = entityutil::getPlayerEntity(registry);
    auto& mc = player.get<MovementComponent>();
    mc.kinematicVelocity.x = moveStickState.x * mc.maxSpeed.x;

    { // handle jump
        static const auto jumpAction = am.getActionTagHash("Jump");
        auto& cc = player.get<CharacterControllerComponent>();
        if (am.isPressed(jumpAction)) {
            cc.wantJump = true;
        }

        // "short jump" - if key is not pressed, more gravity is applied,
        // so the jump becomes shorter
        const auto playerGravity = 1.f;
        // ShortJumpFactor is a factor by which gravity is multiplied if player doesn't hold a
        // jump button.
        const float ShortJumpFactor = 7.5f;
        if (mc.kinematicVelocity.y < 0 && !am.isHeld(jumpAction)) {
            cc.gravity = playerGravity * (1 + ShortJumpFactor);
        } else {
            cc.gravity = playerGravity;
        }
    }

    // handle interation
    static const auto interactAction = am.getActionTagHash("Interact");
    interactEntity = entityutil::findInteractableEntity(registry);
    if (am.wasJustPressed(interactAction) && interactEntity.entity() != entt::null) {
        handleInteraction();
    }
}

void Game::handleInteraction()
{
    auto player = entityutil::getPlayerEntity(registry);
    entityutil::stopMovement(player);

    const auto it = interactEntity.get<InteractComponent>().type;
    switch (it) {
    case InteractComponent::Type::GoInside: {
        const auto& tc = interactEntity.get<TeleportComponent>();
        changeLevel(tc.levelTag, tc.spawnTag);
    } break;
    case InteractComponent::Type::Talk:
        /* fallthrough */
    case InteractComponent::Type::Examine: {
        const auto& npcc = interactEntity.get<NPCComponent>();
        if (!npcc.defaultText.empty()) {
            auto l = say(npcc.defaultText, interactEntity);
            actionListManager.addActionList(std::move(l));
        }
    } break;
    default:
        break;
    }
}

glm::vec2 Game::getMouseGameScreenPos() const
{
    const auto& mousePos = inputManager.getMouse().getPosition();
    return edbr::util::
        getGameWindowScreenCoord(mousePos, gameWindowPos, gameWindowSize, params.renderSize);
}

glm::vec2 Game::getMouseWorldPos() const
{
    return getMouseGameScreenPos() + gameCamera.getPosition2D();
}

void Game::customDraw()
{
    auto cmd = gfxDevice.beginFrame();

    const auto& drawImage = gfxDevice.getImage(drawImageId);
    vkutil::transitionImage(
        cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    // const auto clearColor = glm::vec4{1.f, 0.f, 1.f, 1.f};
    const auto clearColor = glm::vec4{0.f, 0.f, 0.f, 1.f};
    vkutil::clearColorImage(cmd, drawImage.getExtent2D(), drawImage.imageView, clearColor);

    // draw world
    spriteRenderer.beginDrawing();
    drawWorld();
    if (isDevEnvironment) {
        devToolsDrawInWorldUI();
    }
    spriteRenderer.endDrawing();
    spriteRenderer.draw(cmd, drawImage, gameCamera.getViewProj());

    // draw UI
    uiRenderer.beginDrawing();
    drawUI();
    uiRenderer.endDrawing();
    uiRenderer.draw(cmd, drawImage);

    // finish frame
    const auto devClearBgColor = edbr::rgbToLinear(97, 120, 159);
    const auto endFrameProps = GfxDevice::EndFrameProps{
        .clearColor = gameDrawnInWindow ? devClearBgColor : LinearColor::Black(),
        .copyImageIntoSwapchain = !gameDrawnInWindow,
        .drawImageBlitRect = {gameWindowPos.x, gameWindowPos.y, gameWindowSize.x, gameWindowSize.y},
        .drawImageLinearBlit = false,
        .drawImGui = drawImGui,
    };

    gfxDevice.endFrame(cmd, drawImage, endFrameProps);
}

void Game::drawWorld()
{
    const auto& tileMap = level.getTileMap();
    const auto minZ = tileMap.getMinLayerZ();
    const auto maxZ = tileMap.getMaxLayerZ();
    assert(maxZ >= minZ);

    bool drewObjects = false;
    for (int z = minZ; z <= maxZ; ++z) {
        tileMapRenderer.drawTileMapLayers(gfxDevice, spriteRenderer, tileMap, z);
        if (z == 0) {
            drawGameObjects();
            drewObjects = true;
        }
    }

    if (!drewObjects) {
        drawGameObjects();
    }
}

void Game::drawGameObjects()
{
    // TODO: sort by Z
    for (const auto&& [e, sc] : registry.view<SpriteComponent>().each()) {
        const auto spritePos = glm::round(entityutil::getWorldPosition2D({registry, e}));
        spriteRenderer.drawSprite(sc.sprite, spritePos);
    }
}

void Game::drawUI()
{
    auto player = entityutil::getPlayerEntity(registry);
    auto uiCtx = GameUI::UIContext{
        .playerPos = entityutil::getWorldPosition2D(player),
        .cameraPos = gameCamera.getPosition2D(),
    };
    if (interactEntity.entity() != entt::null) {
        auto& ic = interactEntity.get<InteractComponent>();
        uiCtx.interactionType = ic.type;
    }

    ui.draw(uiRenderer, uiCtx);

    if (isDevEnvironment) {
        uiInspector.draw(uiRenderer);
    }
}

void Game::changeLevel(const std::string& levelName, const std::string& spawnName)
{
    newLevelToLoad = levelName;
    newLevelSpawnName = spawnName;
}

void Game::doLevelChange()
{
    bool hasPreviousLevel = !level.getName().empty();
    if (hasPreviousLevel) {
        exitLevel();
    }
    level.load("assets/levels/" + newLevelToLoad + ".json", gfxDevice);

    spawnLevelEntities();
    entityutil::spawnPlayer(registry, newLevelSpawnName);
    enterLevel();

    newLevelToLoad.clear();
    newLevelSpawnName.clear();
}

void Game::enterLevel()
{}

void Game::exitLevel()
{
    destroyNonPersistentEntities();
}

void Game::spawnLevelEntities()
{
    for (const auto& spawner : level.getSpawners()) {
        if (!entityFactory.prefabExists(spawner.prefabName)) {
            fmt::println("skipping prefab '{}': not loaded", spawner.prefabName);
            continue;
        }

        auto e = createEntityFromPrefab(spawner.prefabName, spawner.prefabData);
        entityutil::setWorldPosition2D(e, spawner.position);
        entityutil::setHeading2D(e, spawner.heading);
        if (!spawner.tag.empty()) {
            entityutil::setTag(e, spawner.tag);
        }
    }
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

entt::handle Game::createEntityFromPrefab(
    const std::string& prefabName,
    const nlohmann::json& overrideData)

{
    try {
        return entityFactory.createEntity(registry, prefabName, overrideData);
    } catch (const std::exception& e) {
        throw std::runtime_error(
            fmt::format("failed to create prefab '{}': {}", prefabName, e.what()));
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

    if (removeFromRegistry) {
        e.destroy();
    }
}

namespace
{
LocalizedStringTag getSpeakerName(entt::handle e)
{
    if (e.entity() == entt::null) {
        return LST{};
    }
    if (auto npcc = e.try_get<NPCComponent>(); npcc) {
        if (!npcc->name.empty()) {
            return npcc->name;
        }
    }
    return LST{};
}
}

ActionList Game::say(const LocalizedStringTag& text, entt::handle speaker)
{
    const auto textToken = dialogue::TextToken{
        .text = text,
    };
    return say(textToken, speaker);
}

ActionList Game::say(const dialogue::TextToken& textToken, entt::handle speaker)
{
    return say({&textToken, 1}, speaker);
}

ActionList Game::say(std::span<const dialogue::TextToken> textTokens, entt::handle speaker)
{
    return actions::say(textManager, ui, textTokens, getSpeakerName(speaker));
}
