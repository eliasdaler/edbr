#include "Game.h"

#include <edbr/Core/JsonFile.h>
#include <edbr/Graphics/Letterbox.h>
#include <edbr/Graphics/Vulkan/Util.h>
#include <edbr/Util/FS.h>
#include <edbr/Util/InputUtil.h>

#include <edbr/Util/ImGuiUtil.h>
#include <imgui.h>

namespace
{
void setSpriteAnimation(
    Sprite& sprite,
    SpriteAnimator& animator,
    const std::unordered_map<std::string, SpriteAnimationData>& animationsData,
    const std::string& animsTag,
    const std::string& animName)
{
    animator.setAnimation(animationsData.at(animsTag).getAnimation(animName), animName);
    animator.animate(sprite, animationsData.at(animsTag).getSpriteSheet());
}
}

Game::Game() : spriteRenderer(gfxDevice), uiRenderer(gfxDevice)
{}

void Game::customInit()
{
    inputManager.getActionMapping().loadActions("assets/data/input_actions.json");
    inputManager.loadMapping("assets/data/input_mapping.json");

    createDrawImage(params.renderSize);
    spriteRenderer.init(drawImageFormat);
    uiRenderer.init(drawImageFormat);

    defaultFont.load(gfxDevice, "assets/fonts/m6x11.ttf", 16, false);

    loadAnimations("assets/animations");

    level.load("assets/levels/LevelTown.json", gfxDevice);

    const auto playerImageId = gfxDevice.loadImageFromFile("assets/images/player.png");
    const auto& playerImage = gfxDevice.getImage(playerImageId);
    playerSprite.setTexture(playerImage);
    playerSprite.setTextureRect({80, 48, 16, 16});

    playerPos = {433.f, 224.f};

    setSpriteAnimation(playerSprite, playerSpriteAnimator, animationsData, "player", "run");
}

void Game::loadAnimations(const std::filesystem::path& animationsDir)
{
    // Automatically load all prefabs from the directory
    // Prefab from "assets/prefabs/npc/guy.json" is named "npc/guy"
    util::foreachFileInDir(animationsDir, [this, &animationsDir](const std::filesystem::path& p) {
        auto relPath = p.lexically_relative(animationsDir);
        const auto animsTag = relPath.replace_extension("").string();
        animationsData[animsTag].load(p);
    });
}

void Game::createDrawImage(const glm::ivec2& drawImageSize)
{
    const auto drawImageExtent = VkExtent3D{
        .width = (std::uint32_t)drawImageSize.x,
        .height = (std::uint32_t)drawImageSize.y,
        .depth = 1,
    };

    VkImageUsageFlags usages{};
    usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    usages |= VK_IMAGE_USAGE_SAMPLED_BIT;

    auto createImageInfo = vkutil::CreateImageInfo{
        .format = drawImageFormat,
        .usage = usages,
        .extent = drawImageExtent,
        .samples = VK_SAMPLE_COUNT_1_BIT,
    };
    drawImageId = gfxDevice.createImage(createImageInfo, "draw image", nullptr, drawImageId);
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

void Game::customCleanup()
{
    gfxDevice.waitIdle();
    spriteRenderer.cleanup();
    uiRenderer.cleanup();
}

void Game::customUpdate(float dt)
{
    handleInput(dt);

    if (!gameDrawnInWindow) {
        const auto blitRect =
            util::calculateLetterbox(params.renderSize, gfxDevice.getSwapchainSize(), true);
        gameWindowPos = {blitRect.x, blitRect.y};
        gameWindowSize = {blitRect.z, blitRect.w};
    }

    playerSpriteAnimator.update(dt);
    playerSpriteAnimator.animate(playerSprite, animationsData.at("player").getSpriteSheet());

    if (!freeCamera) {
        cameraPos = playerPos - static_cast<glm::vec2>(params.renderSize) / 2.f;
    }

    if (ImGui::Begin("Hello, world")) {
        ImGui::Drag("playerPos", &playerPos);
        ImGui::End();
    }
}

void Game::handleInput(float dt)
{
    if (!freeCamera) {
        handlePlayerInput(dt);
    } else {
        handleFreeCameraInput(dt);
    }

    if (inputManager.getKeyboard().wasJustPressed(SDL_SCANCODE_C)) {
        freeCamera = !freeCamera;
    }
}

void Game::handlePlayerInput(float dt)
{
    const auto& actionMapping = inputManager.getActionMapping();
    static const auto horizonalWalkAxis = actionMapping.getActionTagHash("MoveX");
    static const auto verticalWalkAxis = actionMapping.getActionTagHash("MoveY");

    const auto moveStickState =
        util::getStickState(actionMapping, horizonalWalkAxis, verticalWalkAxis);

    const auto playerMoveSpeed = glm::vec2{60.f, 60.f};
    auto velocity = moveStickState * playerMoveSpeed;
    playerPos += velocity * dt;
}

void Game::handleFreeCameraInput(float dt)
{
    const auto& actionMapping = inputManager.getActionMapping();
    static const auto moveXAxis = actionMapping.getActionTagHash("CameraMoveX");
    static const auto moveYAxis = actionMapping.getActionTagHash("CameraMoveY");

    const auto moveStickState = util::getStickState(actionMapping, moveXAxis, moveYAxis);
    const auto cameraMoveSpeed = glm::vec2{120.f, 120.f};
    auto velocity = moveStickState * cameraMoveSpeed;
    if (inputManager.getKeyboard().isHeld(SDL_SCANCODE_LSHIFT)) {
        velocity *= 2.f;
    }
    cameraPos += velocity * dt;
}

void Game::customDraw()
{
    auto cmd = gfxDevice.beginFrame();

    const auto devClearBgColor = edbr::rgbToLinear(97, 120, 159);
    const auto endFrameProps = GfxDevice::EndFrameProps{
        .clearColor = gameDrawnInWindow ? devClearBgColor : LinearColor::Black(),
        .copyImageIntoSwapchain = !gameDrawnInWindow,
        .drawImageBlitRect = {gameWindowPos.x, gameWindowPos.y, gameWindowSize.x, gameWindowSize.y},
        .drawImageLinearBlit = false,
        .drawImGui = drawImGui,
    };

    const auto& drawImage = gfxDevice.getImage(drawImageId);
    vkutil::transitionImage(
        cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vkutil::clearColorImage(
        cmd, drawImage.getExtent2D(), drawImage.imageView, glm::vec4{1.f, 0.f, 1.f, 1.f});

    spriteRenderer.beginDrawing();
    drawWorld();
    spriteRenderer.endDrawing();
    spriteRenderer.draw(cmd, drawImage, cameraPos);

    uiRenderer.beginDrawing();
    drawUI();
    uiRenderer.endDrawing();
    uiRenderer.draw(cmd, drawImage);

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
        tileMapRenderer.drawTileMapLayer(gfxDevice, spriteRenderer, tileMap, z);
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
    spriteRenderer.drawSprite(playerSprite, playerPos);
}

void Game::drawUI()
{
    uiRenderer.drawText(defaultFont, "Platformer test", glm::vec2{0.f, 0.f}, {1.f, 1.f, 0.f});
}
