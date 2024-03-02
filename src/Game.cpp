#include "Game.h"

#include <SDL2/SDL.h>

#include <chrono>
#include <iostream>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

#include <Graphics/Scene.h>

#include <tracy/Tracy.hpp>

#ifdef TRACY_ENABLE
void* operator new(std ::size_t count)
{
    auto ptr = malloc(count);
    TracyAlloc(ptr, count);
    return ptr;
}
void operator delete(void* ptr) noexcept
{
    TracyFree(ptr);
    free(ptr);
}
void operator delete(void* ptr, std::size_t) noexcept
{
    TracyFree(ptr);
    free(ptr);
}
#endif

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
        // const auto scene = renderer.loadScene("assets/levels/house/house.gltf");
        const auto scene = renderer.loadScene("assets/levels/city/city.gltf");
        createEntitiesFromScene(scene);
    }

    {
        const auto scene = renderer.loadScene("assets/models/cato.gltf");

        { // create player
            createEntitiesFromScene(scene);

            const glm::vec3 catoPos{1.4f, 0.f, 0.f};
            auto& cato = findEntityByName("Cato");
            cato.tag = "Player";
            cato.transform.position = catoPos;
            cato.skeletonAnimator.setAnimation(cato.skeleton, cato.animations.at("Run"));
        }

        { // create second cato
            createEntitiesFromScene(scene);

            const glm::vec3 catoPos{2.4f, 0.f, 0.f};
            auto& cato = findEntityByName("Cato");
            cato.tag = "Cato2";
            cato.transform.position = catoPos;

            cato.skeletonAnimator.setAnimation(cato.skeleton, cato.animations.at("Walk"));
        }

        { // create third cato
            createEntitiesFromScene(scene);

            const glm::vec3 catoPos{0.4f, 0.f, 0.f};
            auto& cato = findEntityByName("Cato");
            cato.tag = "Cato3";
            cato.transform.position = catoPos;

            cato.skeletonAnimator.setAnimation(cato.skeleton, cato.animations.at("Think"));
        }
    }

    {
        static const float zNear = 1.f;
        static const float zFar = 64.f;
        static const float fovX = glm::radians(45.f);
        static const float aspectRatio = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;

        camera.setUseInverseDepth(true);
        camera.init(fovX, zNear, zFar, aspectRatio);

        const auto startPos = glm::vec3{8.9f, 4.09f, 8.29f};
        camera.setPosition(startPos);
    }
    cameraController.setYawPitch(-2.5f, 0.2f);

    const auto startPos = glm::vec3{-48.8440704, 5.05302525, 5.56558323};
    camera.setPosition(startPos);
    cameraController.setYawPitch(3.92699075, 0.523598909);

    /* const auto startPos = glm::vec3{-48.68f, 16.39f, 17.33f};
    camera.setPosition(startPos);
    cameraController.setYawPitch(3.29f, 1.22f); */

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
            ZoneScopedN("Tick");

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

        {
            ZoneScopedN("Render");
            const auto sceneData = RendererSceneData{
                .camera = camera,
                .ambientColorAndIntensity = ambientColorAndIntensity,
                .sunlightDirection = sunlightDir,
                .sunlightColorAndIntensity = sunlightColorAndIntensity,
            };
            renderer.draw(camera, sceneData);
            FrameMark;
        }

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
    ZoneScopedN("Update");

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
                renderer.getRenderer().destroyBuffer(skinnedMesh.skinnedVertexBuffer);
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

            e.skinnedMeshes.reserve(e.meshes.size());
            for (const auto meshId : e.meshes) {
                e.skinnedMeshes.push_back(renderer.getRenderer().createSkinnedMeshBuffer(meshId));
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
    ZoneScopedN("Update entity transforms");

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
    ZoneScopedN("Generate draw list");

    renderer.beginDrawing();

    for (const auto& ePtr : entities) {
        const auto& e = *ePtr;
        if (e.hasSkeleton) {
            // This interface is not perfect:
            // 1. Not all meshes for the entity might be skinned
            // 2. Different meshes can have different joint matrices sets
            // But right now it's better to group meshes to not waste memory
            renderer.addDrawSkinnedMeshCommand(
                e.meshes, e.skinnedMeshes, e.worldTransform, e.skeletonAnimator.getJointMatrices());
        } else {
            for (const auto id : e.meshes) {
                renderer.addDrawCommand(id, e.worldTransform);
            }
        }
    }

    renderer.endDrawing();
}
