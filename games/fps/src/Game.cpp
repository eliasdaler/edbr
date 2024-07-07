#include "Game.h"

#include <edbr/Core/JsonFile.h>
#include <edbr/Graphics/Letterbox.h>
#include <edbr/Graphics/Vulkan/Util.h>

#include <edbr/Graphics/Scene.h>
#include <edbr/Util/GltfLoader.h>

Game::Game()
{}

void Game::customInit()
{
    drawImageId =
        gfxDevice.createDrawImage(drawImageFormat, params.renderSize, "game screen draw image");

    defaultFont.load(gfxDevice, "assets/fonts/pressstart2p.ttf", 32, false);

    spriteRenderer.init(gfxDevice, drawImageFormat);

    materialCache.init(gfxDevice);
    gameRenderer.init(gfxDevice, params.renderSize);

    std::filesystem::path scenePath{"assets/models/cato.gltf"};
    auto scene = util::loadGltfFile(gfxDevice, meshCache, materialCache, scenePath);
    auto meshIndex = scene.nodes[0]->meshIndex;
    auto mesh = scene.meshes[meshIndex];
    for (const auto& m : mesh.primitives) {
        std::cout << m << ", num vertices = " << scene.cpuMeshes[m].vertices.size() << std::endl;
    }

    catMeshes = mesh.primitives;
    catMaterials = mesh.primitiveMaterials;

    float cameraNear{1.f};
    float cameraFar{200.f};
    float cameraFovX{glm::radians(45.f)};
    auto aspectRatio = (float)params.renderSize.x / (float)params.renderSize.y;
    camera.init(cameraFovX, cameraNear, cameraFar, aspectRatio);
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

    gameRenderer.cleanup(gfxDevice);
    meshCache.cleanup(gfxDevice);
    materialCache.cleanup(gfxDevice);
}

void Game::customUpdate(float dt)
{
    if (!gameDrawnInWindow) {
        const auto& finalDrawImage = gfxDevice.getImage(drawImageId);
        const auto blitRect = util::
            calculateLetterbox(finalDrawImage.getSize2D(), gfxDevice.getSwapchainSize(), true);
        gameWindowPos = {blitRect.x, blitRect.y};
        gameWindowSize = {blitRect.z, blitRect.w};
    }
}

void Game::customDraw()
{
    auto cmd = gfxDevice.beginFrame();

    const auto& drawImage = gfxDevice.getImage(drawImageId);
    vkutil::transitionImage(
        cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // clear screen
    const auto clearColor = glm::vec4{0.f, 1.f, 0.f, 1.f};
    vkutil::clearColorImage(cmd, drawImage.getExtent2D(), drawImage.imageView, clearColor);

    drawGameWorld(cmd);

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

void Game::drawGameWorld(VkCommandBuffer cmd)
{
    GameRenderer::SceneData sceneData{
        .camera = camera,
    };
    gameRenderer.draw(cmd, gfxDevice, meshCache, materialCache, camera, sceneData);
}

void Game::drawUI()
{
    spriteRenderer
        .drawText(gfxDevice, defaultFont, "Hello, world", glm::vec2{64, 64}, LinearColor::Black());
}
