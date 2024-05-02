#include "Game.h"

#include <edbr/Core/JsonFile.h>
#include <edbr/Graphics/Letterbox.h>
#include <edbr/Graphics/Vulkan/Util.h>

#include <imgui.h>

Game::Game() : spriteRenderer(gfxDevice)
{}

void Game::customInit()
{
    createDrawImage(params.renderSize);
    spriteRenderer.init(drawImageFormat);
    defaultFont.load(gfxDevice, "assets/fonts/m6x11.ttf", 16, false);

    const auto playerImageId = gfxDevice.loadImageFromFile("assets/images/player.png");
    const auto& playerImage = gfxDevice.getImage(playerImageId);
    playerSprite.setTexture(playerImage);
    playerSprite.setTextureRect({48, 64, 16, 16});
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
}

void Game::customUpdate(float dt)
{
    if (!gameDrawnInWindow) {
        const auto blitRect =
            util::calculateLetterbox(params.renderSize, gfxDevice.getSwapchainSize(), true);
        gameWindowPos = {blitRect.x, blitRect.y};
        gameWindowSize = {blitRect.z, blitRect.w};
    }

    if (ImGui::Begin("Hello, world")) {
        ImGui::TextUnformatted("This works");
        ImGui::End();
    }
}

void Game::customDraw()
{
    auto cmd = gfxDevice.beginFrame();

    const auto endFrameProps = GfxDevice::EndFrameProps{
        .clearColor =
            gameDrawnInWindow ? edbr::rgbToLinear(97, 120, 159) : LinearColor{0.f, 0.f, 0.f, 1.f},
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
    spriteRenderer.drawSprite(playerSprite, glm::vec2{32.f, 32.f});
    spriteRenderer.drawText(defaultFont, "Hello, world", glm::vec2{100.f, 100.f}, {});
    spriteRenderer.endDrawing();

    spriteRenderer.draw(cmd, drawImage);

    gfxDevice.endFrame(cmd, drawImage, endFrameProps);
}
