#include "Game.h"

#include <edbr/Core/JsonFile.h>
#include <edbr/Graphics/Letterbox.h>
#include <edbr/Graphics/Vulkan/Util.h>

void Game::customInit()
{
    gameScreenDrawImageId =
        gfxDevice.createDrawImage(drawImageFormat, params.renderSize, "game screen draw image");

    defaultFont.load(gfxDevice, "assets/fonts/pressstart2p.ttf", 32, false);

    spriteRenderer.init(gfxDevice, drawImageFormat);
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
}

void Game::customUpdate(float dt)
{
    if (!gameDrawnInWindow) {
        const auto& finalDrawImage = gfxDevice.getImage(gameScreenDrawImageId);
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
    const auto clearColor = glm::vec4{0.f, 1.f, 0.f, 1.f};
    vkutil::clearColorImage(cmd, drawImage.getExtent2D(), drawImage.imageView, clearColor);

    drawGameWorld();

    spriteRenderer.beginDrawing();
    drawUI();
    spriteRenderer.endDrawing();
    spriteRenderer.draw(cmd, gfxDevice, drawImage);

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

void Game::drawGameWorld()
{}

void Game::drawUI()
{
    spriteRenderer
        .drawText(gfxDevice, defaultFont, "Hello, world", glm::vec2{64, 64}, LinearColor::Black());
}
