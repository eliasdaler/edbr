#include "Game.h"

#include "Components.h"
#include "EntityUtil.h"
#include "GameSceneLoader.h"

#include <DevTools/ImGuiPropertyTable.h>
#include <ECS/Components/MetaInfoComponent.h>
#include <Graphics/Scene.h>
#include <Util/InputUtil.h>

#include <chrono>
#include <iostream>

#include <SDL2/SDL.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

#include <Util/FS.h>
#include <Util/ImGuiUtil.h>

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
}

namespace eu = entityutil;

Game::Game() : renderer(gfxDevice), skyboxDir("assets/images/skybox")
{}

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

    gfxDevice.init(window, vSync);
    renderer.init();

    initEntityFactory();
    registerComponents(entityFactory.getComponentFactory());
    registerComponentDisplayers();

    ui.init(renderer.getBaseRenderer());

    { // create camera
        static const float zNear = 1.f;
        static const float zFar = 64.f;
        static const float fovX = glm::radians(45.f);
        static const float aspectRatio = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;

        camera.setUseInverseDepth(true);
        camera.init(fovX, zNear, zFar, aspectRatio);
    }

    // set default camera pos (TODO: get camera from scene or just use follow camera
    // if player exists)
    const auto startPos = glm::vec3{-45.f, 8.29f, 13.17f};
    camera.setPosition(startPos);
    camera.setHeading(glm::quat{0.39f, 0.08f, -0.98f, 0.19f});

    { // create player
        auto e = entityFactory.createEntity(registry, "cato");
        e.emplace<PlayerComponent>();
        // eu::setAnimation(e, "Think");
    }

    // loadLevel("assets/levels/house.json");
    loadLevel("assets/levels/city.json");

    // spawn player
    const auto& spawnName = level.getDefaultPlayerSpawnerName();
    if (!spawnName.empty()) {
        eu::spawnPlayer(registry, level.getDefaultPlayerSpawnerName());
    }

    // set camera
    const auto& cameraName = level.getDefaultCameraName();
    if (!cameraName.empty()) {
        setCurrentCamera(eu::findCameraByName(registry, cameraName));
    } else {
        auto player = eu::getPlayerEntity(registry);
        if (player.entity() != entt::null) {
            fmt::println("No default camera found: using follow camera");

            orbitCameraAroundSelectedEntity = true;

            // follow cam
            camera.setYawPitch(glm::radians(-135.f), glm::radians(-20.f));
            entityTreeView.setSelectedEntity(player);
        } else {
            // try to find camera named "Default"
            auto cameraEnt = findDefaultCamera();
            if (cameraEnt.entity() != entt::null) {
                setCurrentCamera(cameraEnt);
            } else {
                fmt::println("No default camera found");
            }
        }
    }

    { // create entities from prefabs
        { // test
            const glm::vec3 treePos{-58.5f, 0.07f, 7.89f};
            auto tree = entityFactory.createEntity(registry, "pine_tree");
            eu::setPosition(tree, treePos);
            setEntityTag(tree, "WiseTree");

            auto interactor = entityFactory.createEntity(registry, "interaction_trigger");
            eu::addChild(tree, interactor);
        }

        { // for city level
            auto npc = eu::findEntityBySceneNodeName(registry, "generic_npc.2");
            if ((bool)npc) {
                setEntityTag(npc, "CoolDude");
            }
        }
    }
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
        if (newFPS == std::numeric_limits<float>::infinity()) {
            // can happen when frameTime == 0
            newFPS = 0;
        }
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
            auto& io = ImGui::GetIO();
            if (!io.WantCaptureKeyboard) {
                handleInput(dt);
            }
            update(dt);

            accumulator -= dt;

            ImGui::Render();
        }

        {
            ZoneScopedN("Generate draw list");
            generateDrawList();
        }

        {
            ZoneScopedN("Render");
            const auto sceneData = GameRenderer::SceneData{
                .camera = camera,
                .ambientColor = level.getAmbientLightColor(),
                .ambientIntensity = level.getAmbientLightIntensity(),
                .fogColor = level.getFogColor(),
                .fogDensity = level.getFogDensity(),
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

    if (orbitCameraAroundSelectedEntity) {
        handlePlayerInput(dt);
    }
}

void Game::handlePlayerInput(float dt)
{
    const auto moveStickState = util::getStickState({
        .up = SDL_SCANCODE_W,
        .down = SDL_SCANCODE_S,
        .left = SDL_SCANCODE_A,
        .right = SDL_SCANCODE_D,
    });

    auto player = eu::getPlayerEntity(registry);
    if (moveStickState != glm::vec2{}) {
        auto& mc = player.get<MovementComponent>();
        auto speed = mc.maxSpeed;
        if (!util::isKeyPressed(SDL_SCANCODE_LSHIFT)) {
            speed /= 3.5f; // TODO: make walk speed configurable
        }

        const auto heading = util::calculateStickHeading(camera, moveStickState);
        mc.velocity = heading * speed;

        // rotate in the direction of movement
        const auto angle = std::atan2(heading.x, heading.z);
        const auto targetHeading = glm::angleAxis(angle, math::GlobalUpAxis);
        auto& tc = player.get<TransformComponent>();
        tc.transform.setHeading(targetHeading);
    } else {
        auto& mc = player.get<MovementComponent>();
        mc.velocity = glm::vec3{};
    }
}

void Game::update(float dt)
{
    ZoneScopedN("Update");

    cameraController.update(camera, dt);

    // camera orbit
    if (orbitCameraAroundSelectedEntity && entityTreeView.hasSelectedEntity()) {
        const auto e = entityTreeView.getSelectedEntity();
        const auto& tc = e.get<TransformComponent>();
        const auto orbitTarget = tc.transform.getPosition() + glm::vec3{0.f, 1.5f, 0.f};
        const auto newPos = orbitTarget - camera.getTransform().getLocalFront() * orbitDistance;
        camera.setPosition(newPos);
    }

    // movement
    for (auto&& [e, tc, mc] : registry.view<TransformComponent, MovementComponent>().each()) {
        const auto newPos = tc.transform.getPosition() + mc.velocity * dt;
        tc.transform.setPosition(newPos);
    }

    updateEntityTransforms();

    // setting animation based on velocity
    // TODO: do this in state machine instead
    for (auto&& [e, mc, sc] : registry.view<MovementComponent, SkeletonComponent>().each()) {
        const auto velMag = glm::length(mc.velocity);
        if (std::abs(velMag) <= 0.001f) {
            const auto& animator = sc.skeletonAnimator;
            if (animator.getCurrentAnimationName() == "Run" ||
                animator.getCurrentAnimationName() == "Walk") {
                eu::setAnimation({registry, e}, "Idle");
            }
            continue;
        }

        if (mc.velocity.x != 0.f && mc.velocity.z != 0.f) {
            if (velMag > 2.f) {
                eu::setAnimation({registry, e}, "Run");
            } else if (velMag > 0.1f) {
                // HACK: sometimes the player can fall slightly when being spawned
                eu::setAnimation({registry, e}, "Walk");
            }
        }
    }

    // animate entities with skeletons
    for (const auto&& [e, sc] : registry.view<SkeletonComponent>().each()) {
        sc.skeletonAnimator.update(sc.skeleton, dt);
    }

    updateDevTools(dt);
}

void Game::updateEntityTransforms()
{
    ZoneScopedN("Update entity transforms");

    const auto I = glm::mat4{1.f};
    for (auto&& [e, hc] : registry.view<HierarchyComponent>().each()) {
        if (!hc.hasParent()) { // start from root nodes
            updateEntityTransforms(e, I);
        }
    }
}

void Game::updateEntityTransforms(entt::entity e, const glm::mat4& parentWorldTransform)
{
    auto [tc, hc] = registry.get<TransformComponent, const HierarchyComponent>(e);
    if (hc.hasParent()) {
        tc.worldTransform = tc.transform.asMatrix();
    } else {
        const auto prevTransform = tc.worldTransform;
        tc.worldTransform = parentWorldTransform * tc.transform.asMatrix();
        if (tc.worldTransform == prevTransform) {
            return;
        }
    }

    for (const auto& child : hc.children) {
        updateEntityTransforms(child, tc.worldTransform);
    }
}

void Game::updateDevTools(float dt)
{
    if (displayFPSDelay > 0.f) {
        displayFPSDelay -= dt;
    } else {
        displayFPSDelay = 1.f;
        displayedFPS = avgFPS;
    }

    if (ImGui::Begin("Debug")) {
        ImGui::Text("FPS: %d", (int)displayedFPS);

        /* TODO
        if (ImGui::Checkbox("VSync", &vSync)) {
        }
        ImGui::Checkbox("Frame limit", &frameLimit);
        */

        using namespace devtools;
        BeginPropertyTable();
        {
            DisplayProperty("Level", level.getPath().string());

            const auto cameraPos = camera.getPosition();
            DisplayProperty("Cam pos", cameraPos);
            DisplayProperty("Cam heading", camera.getHeading());
        }
        EndPropertyTable();

        ImGui::Checkbox("Orbit around selected entity", &orbitCameraAroundSelectedEntity);
        ImGui::DragFloat("Orbit distance", &orbitDistance, 0.01f, 0.f, 100.f);

        auto ambientColor = level.getAmbientLightColor();
        if (util::ImGuiColorEdit3("Ambient", ambientColor)) {
            level.setAmbientLightColor(ambientColor);
        }
        auto ambientIntensity = level.getAmbientLightIntensity();
        if (ImGui::DragFloat("Ambient intensity", &ambientIntensity, 1.f, 0.f, 1.f)) {
            level.setAmbientLightIntensity(ambientIntensity);
        }

        auto fogColor = level.getFogColor();
        if (util::ImGuiColorEdit3("Fog", fogColor)) {
            level.setFogColor(fogColor);
        }
        auto fogDensity = level.getFogDensity();
        if (ImGui::DragFloat("Fog density", &fogDensity, 1.f, 0.f, 1.f)) {
            level.setFogDensity(fogDensity);
        }

        renderer.updateDevTools(dt);
        ui.updateDevTools(dt);
    }
    ImGui::End();

    if (ImGui::Begin("Entities")) {
        entityTreeView.update(registry, dt);
    }
    ImGui::End();

    if (entityTreeView.hasSelectedEntity()) {
        if (ImGui::Begin("Selected entity")) {
            entityInfoDisplayer.displayEntityInfo(entityTreeView.getSelectedEntity(), dt);
        }
        ImGui::End();
    }
}

void Game::cleanup()
{
    registry.clear();

    renderer.cleanup();
    gfxDevice.cleanup();

    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Game::setEntityTag(entt::handle e, const std::string& tag)
{
    auto& tc = e.get<TagComponent>();
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

void Game::setCurrentCamera(entt::const_handle cameraEnt)
{
    if (cameraEnt.entity() == entt::null) {
        fmt::println("Passed null into setCurrentCamera");
        return;
    }
    if (auto ncPtr = cameraEnt.try_get<NameComponent>(); ncPtr) {
        fmt::println("Current camera: {}", ncPtr->name);
    } else {
        fmt::println("Current camera: {}", cameraEnt.get<MetaInfoComponent>().sceneNodeName);
    }
    auto& tc = cameraEnt.get<TransformComponent>();
    camera.setPosition(tc.transform.getPosition());
    camera.setHeading(tc.transform.getHeading());
}

entt::const_handle Game::findDefaultCamera()
{
    auto cameraEnt = eu::findCameraByName(registry, "Default");
    if (cameraEnt.entity() != entt::null) {
        return cameraEnt;
    }
    return eu::findEntityBySceneNodeName(registry, "Camera");
}

namespace
{
bool shouldCastShadow(const entt::registry& registry, entt::entity e)
{
    const auto& tag = registry.get<TagComponent>(e).getTag();
    return !tag.starts_with("GroundTile");
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
        entt::exclude<SkeletonComponent, TriggerComponent>);
    for (const auto&& [e, tc, mc] : staticMeshes.each()) {
        bool castShadow = shouldCastShadow(registry, e);
        for (std::size_t i = 0; i < mc.meshes.size(); ++i) {
            const auto meshTransform = mc.meshTransforms[i].isIdentity() ?
                                           tc.worldTransform :
                                           tc.worldTransform * mc.meshTransforms[i].asMatrix();
            renderer.drawMesh(mc.meshes[i], meshTransform, castShadow);
        }
    }

    // render meshes with skeletal animation
    const auto skinnedMeshes =
        registry.view<TransformComponent, MeshComponent, SkeletonComponent>();
    for (const auto&& [e, tc, mc, sc] : skinnedMeshes.each()) {
        // 1. Not all meshes for the entity might be skinned
        // 2. Different meshes can have different joint matrices sets
        // But right now it's better to group meshes to not waste memory
        // It's also assumed that all skinned meshes have meshTransform[i] == I
#ifndef NDEBUG
        for (const auto& t : mc.meshTransforms) {
            assert(t.isIdentity());
        }
#endif

        renderer.drawSkinnedMesh(
            mc.meshes, sc.skinnedMeshes, tc.worldTransform, sc.skeletonAnimator.getJointMatrices());
    }

    auto& uiRenderer = renderer.getSpriteRenderer();
    ui.draw(uiRenderer);

    renderer.endDrawing();
}

void Game::loadLevel(const std::filesystem::path& path)
{
    level.load(path);

    // load skybox
    if (level.hasSkybox()) {
        const auto skyboxImageId =
            renderer.getBaseRenderer().loadCubemap(skyboxDir / level.getSkyboxName());
        renderer.setSkyboxImage(skyboxImageId);
    } else {
        // no skybox
        renderer.setSkyboxImage(NULL_IMAGE_ID);
    }

    loadScene(level.getSceneModelPath(), true);
}

void Game::loadScene(const std::filesystem::path& path, bool createEntities)
{
    const auto& scene = sceneCache.loadScene(renderer.getBaseRenderer(), path);
    if (createEntities) {
        const auto loadCtx = util::SceneLoadContext{
            .renderer = renderer,
            .entityFactory = entityFactory,
            .registry = registry,
            .sceneCache = sceneCache,
            .defaultPrefabName = "static_geometry",
        };
        util::createEntitiesFromScene(loadCtx, scene);
    }
}

void Game::onTagComponentDestroy(entt::registry& registry, entt::entity entity)
{
    auto& tc = registry.get<TagComponent>(entity);
    taggedEntities.erase(tc.tag);
}

void Game::onSkeletonComponentDestroy(entt::registry& registry, entt::entity entity)
{
    auto& sc = registry.get<SkeletonComponent>(entity);
    for (const auto& skinnedMesh : sc.skinnedMeshes) {
        renderer.getGfxDevice().destroyBuffer(skinnedMesh.skinnedVertexBuffer);
    }
}

void Game::initEntityFactory()
{
    entityFactory.setCreateDefaultEntityFunc([](entt::registry& registry) {
        auto e = registry.create();
        registry.emplace<TransformComponent>(e);
        registry.emplace<HierarchyComponent>(e);
        registry.emplace<TagComponent>(e);
        return entt::handle(registry, e);
    });

    entityFactory.setPostInitEntityFunc([this](entt::handle e) { postInitEntity(e); });

    const auto prefabsDir = std::filesystem::path{"assets/prefabs"};
    loadPrefabs(prefabsDir);

    entityFactory.addMappedPrefabName("collision", "trigger"); // temp
    entityFactory.addMappedPrefabName("interact", "trigger");
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

void Game::registerComponents(ComponentFactory& componentFactory)
{
    registry.on_destroy<TagComponent>().connect<&Game::onTagComponentDestroy>(this);
    registry.on_destroy<SkeletonComponent>().connect<&Game::onSkeletonComponentDestroy>(this);

    componentFactory.registerComponent<TriggerComponent>("trigger");
    componentFactory.registerComponent<CameraComponent>("camera");
    componentFactory.registerComponent<PlayerSpawnComponent>("player_spawn");

    componentFactory.registerComponentLoader(
        "mesh", [](entt::handle e, MeshComponent& mc, const JsonDataLoader& loader) {
            loader.get("mesh", mc.meshPath);
        });

    componentFactory.registerComponentLoader(
        "movement", [](entt::handle e, MovementComponent& mc, const JsonDataLoader& loader) {
            loader.get("maxSpeed", mc.maxSpeed);
        });
}

namespace
{
std::string extractNameFromSceneNodeName(const std::string& sceneNodeName)
{
    if (sceneNodeName.empty()) {
        return {};
    }
    const auto dotPos = sceneNodeName.find_first_of(".");
    if (dotPos == std::string::npos || dotPos == sceneNodeName.length() - 1) {
        fmt::println(
            "ERROR: cannot extract name {}: should have format <prefab>.<name>", sceneNodeName);
    }
    return sceneNodeName.substr(dotPos + 1, sceneNodeName.size());
}
}

void Game::postInitEntity(entt::handle e)
{
    if (auto mcPtr = e.try_get<MeshComponent>(); mcPtr) {
        auto& mc = *mcPtr;

        const auto& scene = sceneCache.loadScene(renderer.getBaseRenderer(), mc.meshPath);
        assert(!scene.nodes.empty());
        const auto& node = *scene.nodes[0];

        mc.meshes.push_back(node.meshIndex);
        mc.meshTransforms.push_back(glm::mat4{1.f});

        const auto& primitives = scene.meshes[node.meshIndex].primitives;
        mc.meshes = primitives;
        mc.meshTransforms.resize(primitives.size());

        // handle non-identity transform of the model
        auto& tc = e.get<TransformComponent>();
        tc.transform = node.transform;

        // merge child meshes into the parent
        // FIXME: this is the same logic as in createEntityFromNode
        // and probably needs to work recursively for all children
        for (const auto& childPtr : node.children) {
            if (!childPtr) {
                continue;
            }
            const auto& childNode = *childPtr;
            auto& mc = e.get_or_emplace<MeshComponent>();
            for (const auto& meshId : scene.meshes[childNode.meshIndex].primitives) {
                mc.meshes.push_back(meshId);
                mc.meshTransforms.push_back(childNode.transform);
            }
        }

        if (node.skinId != -1) { // add skeleton component if skin exists
            auto& sc = e.get_or_emplace<SkeletonComponent>();
            sc.skeleton = scene.skeletons[node.skinId];
            // FIXME: this is bad - we need to have some sort of cache
            // and not copy animations everywhere
            sc.animations = scene.animations;

            sc.skinnedMeshes.reserve(mc.meshes.size());
            for (const auto meshId : mc.meshes) {
                sc.skinnedMeshes.push_back(renderer.createSkinnedMesh(meshId));
            }

            if (sc.animations.contains("Idle")) {
                sc.skeletonAnimator.setAnimation(sc.skeleton, sc.animations.at("Idle"));
            }
        }
    }

    // extract player spawn name from scene node name
    if (e.all_of<PlayerSpawnComponent>()) {
        const auto& sceneNodeName = e.get<MetaInfoComponent>().sceneNodeName;
        if (sceneNodeName.empty()) { // created manually
            return;
        }
        e.get_or_emplace<NameComponent>().name = extractNameFromSceneNodeName(sceneNodeName);
    }

    // extract camera name from scene node name
    if (e.all_of<CameraComponent>()) {
        const auto& sceneNodeName = e.get<MetaInfoComponent>().sceneNodeName;
        if (sceneNodeName.empty()) { // created manually
            return;
        }
        e.get_or_emplace<NameComponent>().name = extractNameFromSceneNodeName(sceneNodeName);
    }
}
