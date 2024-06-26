#include "Game.h"

#include <edbr/Core/JsonFile.h>
#include <edbr/Graphics/Letterbox.h>
#include <edbr/Graphics/Vulkan/Util.h>

#include <edbr/TileMap/TileMapRenderer.h>
#include <edbr/TileMap/TiledImporter.h>

void Game::customInit()
{
    gameScreenDrawImageId =
        gfxDevice.createDrawImage(drawImageFormat, params.renderSize, "game screen draw image");

    // this will create finalDrawImage
    onWindowResize();

    crtPipeline.init(gfxDevice, drawImageFormat);

    spriteRenderer.init(gfxDevice, drawImageFormat);

    auto playerTextureId = gfxDevice.loadImageFromFile("assets/images/player.png");
    playerSprite.setTexture(gfxDevice.getImage(playerTextureId));

    auto fakeUITextureId = gfxDevice.loadImageFromFile("assets/images/fake_ui.png");
    fakeUISprite.setTexture(gfxDevice.getImage(fakeUITextureId));

    defaultFont.load(gfxDevice, "assets/fonts/pressstart2p.ttf", 8, false);

    TiledImporter importer;
    const auto levelPath = std::filesystem::path{"assets/levels/subway.tmj"};
    importer.loadLevel(gfxDevice, levelPath, tileMap);
}

void Game::onWindowResize()
{
    bool integerScale = false;
    const auto blitRect =
        util::calculateLetterbox(params.renderSize, params.windowSize, integerScale);
    const auto finalDrawImageSize = glm::ivec2{blitRect.z, blitRect.w};

    if (finalDrawImageId != NULL_IMAGE_ID) { // destroy previous image
        const auto& finalDrawImage = gfxDevice.getImage(finalDrawImageId);
        if (finalDrawImage.getSize2D() == finalDrawImageSize) {
            // same size - no need to re-create
            return;
        }
        gfxDevice.destroyImage(finalDrawImage);
    }

    finalDrawImageId = gfxDevice.createDrawImage(
        drawImageFormat, finalDrawImageSize, "final draw image", finalDrawImageId);
}

void Game::loadAppSettings()
{
    const std::filesystem::path appSettingsPath{"assets/data/default_app_settings.json"};
    JsonFile file(appSettingsPath);
    if (!file.isGood()) {
        fmt::println("failed to load app settings from {}", appSettingsPath.string());
        return;
    }

    const auto loader = file.getLoader();
    loader.getIfExists("renderResolution", params.renderSize);
    loader.getIfExists("vSync", vSync);
    if (params.windowSize == glm::ivec2{}) {
        loader.getIfExists("windowSize", params.windowSize);
    } // otherwise it was already set by dev settings

    params.version = Version{
        .major = 0,
        .minor = 1,
        .patch = 0,
    };
}

void Game::customCleanup()
{
    gfxDevice.waitIdle();
    spriteRenderer.cleanup(gfxDevice);
    crtPipeline.cleanup(gfxDevice.getDevice());
}

void Game::customUpdate(float dt)
{
    if (!gameDrawnInWindow) {
        const auto& finalDrawImage = gfxDevice.getImage(finalDrawImageId);
        const auto blitRect = util::
            calculateLetterbox(finalDrawImage.getSize2D(), gfxDevice.getSwapchainSize(), true);
        gameWindowPos = {blitRect.x, blitRect.y};
        gameWindowSize = {blitRect.z, blitRect.w};
    }
}

void Game::customDraw()
{
    auto cmd = gfxDevice.beginFrame();

    const auto& drawImage = gfxDevice.getImage(gameScreenDrawImageId);
    vkutil::transitionImage(
        cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // clear screen
    const auto clearColor = glm::vec4{1.f, 1.f, 0.f, 1.f};
    vkutil::clearColorImage(cmd, drawImage.getExtent2D(), drawImage.imageView, clearColor);

    // draw sprites
    spriteRenderer.beginDrawing();
    {
        drawGameWorld();
        drawUI();
    }
    spriteRenderer.endDrawing();
    spriteRenderer.draw(cmd, gfxDevice, drawImage);

    // apply CRT effect
    const auto& finalDrawImage = gfxDevice.getImage(finalDrawImageId);
    {
        ZoneScopedN("CRT");
        vkutil::cmdBeginLabel(cmd, "CRT");
        crtPipeline.draw(cmd, gfxDevice, drawImage, finalDrawImage);
        vkutil::cmdEndLabel(cmd);
    }

    // finish frame
    const auto devClearBgColor = edbr::rgbToLinear(97, 120, 159);
    const auto endFrameProps = GfxDevice::EndFrameProps{
        .clearColor = gameDrawnInWindow ? devClearBgColor : LinearColor::Black(),
        .copyImageIntoSwapchain = !gameDrawnInWindow,
        .drawImageBlitRect = {gameWindowPos.x, gameWindowPos.y, gameWindowSize.x, gameWindowSize.y},
        .drawImageLinearBlit = false,
        .drawImGui = drawImGui,
    };
    gfxDevice.endFrame(cmd, finalDrawImage, endFrameProps);
}

void Game::drawGameWorld()
{
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
    playerPos = glm::vec2{64.f, 140.f};
    spriteRenderer.drawSprite(gfxDevice, playerSprite, playerPos);
}

void Game::drawUI()
{
    spriteRenderer.drawSprite(gfxDevice, fakeUISprite, glm::vec2{0.f, 0.f});
    /* spriteRenderer
        .drawText(defaultFont, "Hello, world", glm::vec2{16, 16}, LinearColor::White()); */
}
