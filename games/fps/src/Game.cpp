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
    defaultFont.load(gfxDevice, "assets/fonts/pressstart2p.ttf", 32, false);

    spriteRenderer.init(gfxDevice, renderer.getDrawImageFormat());

    materialCache.init(gfxDevice);
    renderer.init(gfxDevice, params.renderSize);

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
    camera.setUseInverseDepth(true);

    camera.setPosition({0.f, 4.5f, 5.f});
    camera.lookAt({0.f, 0.f, 0.f});
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

    renderer.cleanup(gfxDevice);
    meshCache.cleanup(gfxDevice);
    materialCache.cleanup(gfxDevice);
}

void Game::customUpdate(float dt)
{
    if (!gameDrawnInWindow) {
        const auto& drawImage = renderer.getDrawImage(gfxDevice);
        const auto blitRect =
            util::calculateLetterbox(drawImage.getSize2D(), gfxDevice.getSwapchainSize(), true);
        gameWindowPos = {blitRect.x, blitRect.y};
        gameWindowSize = {blitRect.z, blitRect.w};
    }
}

void Game::customDraw()
{
    auto cmd = gfxDevice.beginFrame();

    drawGameWorld(cmd);
    const auto& drawImage = renderer.getDrawImage(gfxDevice);

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
    renderer.beginDrawing(gfxDevice);

    for (std::size_t i = 0; i < catMeshes.size(); ++i) {
        renderer.drawMesh(meshCache, catMeshes[i], glm::mat4{1.f}, catMaterials[i], false);
    }
    renderer.endDrawing();

    GameRenderer::SceneData sceneData{
        .camera = camera,
        .ambientColor = LinearColor::White(),
        .ambientIntensity = 1.f,
    };

    renderer.draw(cmd, gfxDevice, meshCache, materialCache, camera, sceneData);
}

void Game::drawUI()
{
    spriteRenderer
        .drawText(gfxDevice, defaultFont, "Hello, world", glm::vec2{64, 64}, LinearColor::White());
}
