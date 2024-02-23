#include "Game.h"

#include <SDL2/SDL.h>

#include <chrono>
#include <iostream>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

#include <Graphics/Scene.h>

namespace
{
static constexpr std::uint32_t SCREEN_WIDTH = 1280;
static constexpr std::uint32_t SCREEN_HEIGHT = 960;
static constexpr auto NO_TIMEOUT = std::numeric_limits<std::uint64_t>::max();
}

void Game::init()
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        std::exit(1);
    }

    window = SDL_CreateWindow(
        "Vulkan app",
        // pos
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        // size
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_VULKAN);

    if (!window) {
        std::cout << "Failed to create window. SDL Error: " << SDL_GetError();
        std::exit(1);
    }

    renderer.init(window, vSync);

    {
        const auto scene = renderer.loadScene("assets/levels/house/house.gltf");
        // loader.loadScene(loadContext, scene, "assets/levels/city/city.gltf");
        createEntitiesFromScene(scene);
    }

    {
        const auto scene = renderer.loadScene("assets/models/cato.gltf");
        createEntitiesFromScene(scene);

        const glm::vec3 catoPos{1.4f, 0.f, 0.f};
        auto& cato = findEntityByName("Cato");
        cato.transform.position = catoPos;
    }

    {
        static const float zNear = 1.f;
        static const float zFar = 1000.f;
        static const float fovX = glm::radians(45.f);
        static const float aspectRatio = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;

        camera.init(fovX, zNear, zFar, aspectRatio);

        const auto startPos = glm::vec3{8.9f, 4.09f, 8.29f};
        camera.setPosition(startPos);
    }
    cameraController.setYawPitch(-2.5f, 0.2f);

    sunlightDir = glm::vec4{0.371477008, 0.470861048, 0.80018419, 0.f};
    sunlightColorAndIntensity = glm::vec4{213.f / 255.f, 136.f / 255.f, 49.f / 255.f, 0.6f};
    ambientColorAndIntensity = glm::vec4{0.20784314, 0.592156887, 0.56078434, 0.05f};
}

void Game::run()
{
    // Fix your timestep! game loop
    const float FPS = 60.f;
    const float dt = 1.f / FPS;

    auto prevTime = std::chrono::high_resolution_clock::now();
    float accumulator = dt; // so that we get at least 1 update before render

    isRunning = true;
    while (isRunning) {
        const auto newTime = std::chrono::high_resolution_clock::now();
        frameTime = std::chrono::duration<float>(newTime - prevTime).count();

        accumulator += frameTime;
        prevTime = newTime;

        // moving average
        float newFPS = 1.f / frameTime;
        avgFPS = std::lerp(avgFPS, newFPS, 0.1f);

        if (accumulator > 10 * dt) { // game stopped for debug
            accumulator = dt;
        }

        while (accumulator >= dt) {
            { // event processing
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    ImGui_ImplSDL2_ProcessEvent(&event);
                    if (event.type == SDL_QUIT) {
                        isRunning = false;
                        return;
                    }
                }
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            // update
            handleInput(dt);
            update(dt);

            accumulator -= dt;

            ImGui::Render();
        }

        generateDrawList();
        renderer.draw(camera);

        if (frameLimit) {
            // Delay to not overload the CPU
            const auto now = std::chrono::high_resolution_clock::now();
            const auto frameTime = std::chrono::duration<float>(now - prevTime).count();
            if (dt > frameTime) {
                SDL_Delay(static_cast<std::uint32_t>(dt - frameTime));
            }
        }
    }
}

void Game::handleInput(float dt)
{
    cameraController.handleInput(camera);
}

void Game::update(float dt)
{
    cameraController.update(camera, dt);
    updateEntityTransforms();

    // animate entities with skeletons
    for (const auto& ePtr : entities) {
        auto& e = *ePtr;
        if (e.hasSkeleton) {
            e.skeletonAnimator.update(e.skeleton, dt);
        }
    }

    updateDevTools(dt);
}

void Game::updateDevTools(float dt)
{
    if (displayFPSDelay > 0.f) {
        displayFPSDelay -= dt;
    } else {
        displayFPSDelay = 1.f;
        displayedFPS = avgFPS;
    }

    ImGui::Begin("Debug");
    ImGui::Text("FPS: %d", (int)displayedFPS);
    if (ImGui::Checkbox("VSync", &vSync)) {
        // TODO
    }
    ImGui::Checkbox("Frame limit", &frameLimit);

    const auto cameraPos = camera.getPosition();
    ImGui::Text("Camera pos: %.2f, %.2f, %.2f", cameraPos.x, cameraPos.y, cameraPos.z);
    const auto yaw = cameraController.getYaw();
    const auto pitch = cameraController.getPitch();
    ImGui::Text("Camera rotation: (yaw) %.2f, (pitch) %.2f", yaw, pitch);

    auto glmToArr = [](const glm::vec4& v) { return std::array<float, 4>{v.x, v.y, v.z, v.w}; };
    auto arrToGLM = [](const std::array<float, 4>& v) { return glm::vec4{v[0], v[1], v[2], v[3]}; };

    auto ambient = glmToArr(ambientColorAndIntensity);
    if (ImGui::ColorEdit3("Ambient", ambient.data())) {
        ambientColorAndIntensity = arrToGLM(ambient);
    }
    ImGui::DragFloat("Ambient intensity", &ambientColorAndIntensity.w, 1.f, 0.f, 1.f);

    auto sunlight = glmToArr(sunlightColorAndIntensity);
    if (ImGui::ColorEdit3("Sunlight", sunlight.data())) {
        sunlightColorAndIntensity = arrToGLM(sunlight);
    }
    ImGui::DragFloat("Sunlight intensity", &sunlightColorAndIntensity.w, 1.f, 0.f, 1.f);

    ImGui::End();

    renderer.updateDevTools(dt);
}

void Game::cleanup()
{
    for (const auto& ePtr : entities) {
        auto& e = *ePtr;
        if (e.hasSkeleton) {
            for (const auto& skinnedMesh : e.skinnedMeshes) {
                renderer.destroyBuffer(skinnedMesh.skinnedVertexBuffer);
            }
        }
    }

    renderer.cleanup();

    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Game::createEntitiesFromScene(const Scene& scene)
{
    for (const auto& nodePtr : scene.nodes) {
        if (nodePtr) {
            createEntitiesFromNode(scene, *nodePtr);
        }
    }
}

Game::EntityId Game::createEntitiesFromNode(
    const Scene& scene,
    const SceneNode& node,
    EntityId parentId)
{
    auto& e = makeNewEntity();
    e.tag = node.name;

    // transform
    {
        e.transform = node.transform;
        if (parentId == NULL_ENTITY_ID) {
            e.worldTransform = e.transform.asMatrix();
        } else {
            const auto& parent = entities[parentId];
            e.worldTransform = parent->worldTransform * node.transform.asMatrix();
        }
    }

    { // mesh
        e.meshes = scene.meshes[node.meshIndex].primitives;
        // skeleton
        if (node.skinId != -1) {
            e.hasSkeleton = true;
            e.skeleton = scene.skeletons[node.skinId];
            // FIXME: this is bad - we need to have some sort of cache
            // and not copy animations everywhere
            e.animations = scene.animations;

            e.skeletonAnimator.setAnimation(e.skeleton, e.animations.at("Run"));
            e.skinnedMeshes.reserve(e.meshes.size());
            for (const auto meshId : e.meshes) {
                e.skinnedMeshes.push_back(renderer.createSkinnedMeshBuffer(meshId));
            }
        }
    }

    { // hierarchy
        e.parentId = parentId;
        for (const auto& childPtr : node.children) {
            if (childPtr) {
                const auto childId = createEntitiesFromNode(scene, *childPtr, e.id);
                e.children.push_back(childId);
            }
        }
    }

    return e.id;
}

Game::Entity& Game::makeNewEntity()
{
    entities.push_back(std::make_unique<Entity>());
    auto& e = *entities.back();
    e.id = entities.size() - 1;
    return e;
}

Game::Entity& Game::findEntityByName(std::string_view name) const
{
    for (const auto& ePtr : entities) {
        if (ePtr->tag == name) {
            return *ePtr;
        }
    }

    throw std::runtime_error(std::string{"failed to find entity with name "} + std::string{name});
}

void Game::updateEntityTransforms()
{
    const auto I = glm::mat4{1.f};
    for (auto& ePtr : entities) {
        auto& e = *ePtr;
        if (e.parentId == NULL_MESH_ID) {
            updateEntityTransforms(e, I);
        }
    }
}

void Game::updateEntityTransforms(Entity& e, const glm::mat4& parentWorldTransform)
{
    const auto prevTransform = e.worldTransform;
    e.worldTransform = parentWorldTransform * e.transform.asMatrix();
    if (e.worldTransform == prevTransform) {
        return;
    }

    for (const auto& childId : e.children) {
        auto& child = *entities[childId];
        updateEntityTransforms(child, e.worldTransform);
    }
}

void Game::generateDrawList()
{
    const auto sceneData = GPUSceneData{
        .view = camera.getView(),
        .proj = camera.getProjection(),
        .viewProj = camera.getViewProj(),
        .cameraPos = glm::vec4{camera.getPosition(), 1.f},
        .ambientColorAndIntensity = ambientColorAndIntensity,
        .sunlightDirection = sunlightDir,
        .sunlightColorAndIntensity = sunlightColorAndIntensity,
    };
    renderer.beginDrawing(sceneData);

    for (const auto& ePtr : entities) {
        const auto& e = *ePtr;
        for (std::size_t i = 0; i < e.meshes.size(); ++i) {
            if (e.hasSkeleton) {
                renderer.addDrawSkinnedMeshCommand(
                    e.meshes[i],
                    e.skinnedMeshes[i],
                    e.worldTransform,
                    e.skeletonAnimator.getJointMatrices());
            } else {
                renderer.addDrawCommand(e.meshes[i], e.worldTransform);
            }
        }
    }

    renderer.endDrawing();
}
