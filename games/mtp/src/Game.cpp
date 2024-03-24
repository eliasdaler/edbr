#include "Game.h"

#include "Components.h"
#include "EntityUtil.h"
#include "GameSceneLoader.h"
#include "Systems.h"

#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/Graphics/Cubemap.h>
#include <edbr/Graphics/Scene.h>
#include <edbr/Util/FS.h>
#include <edbr/Util/InputUtil.h>

#include <tracy/Tracy.hpp>

namespace eu = entityutil;

Game::Game() :
    Application(),
    baseRenderer(gfxDevice),
    renderer(gfxDevice, baseRenderer),
    spriteRenderer(gfxDevice),
    entityInitializer(sceneCache, renderer)
{}

void Game::customInit()
{
    baseRenderer.init();
    renderer.init();
    spriteRenderer.init(renderer.getDrawImageFormat());

    skyboxDir = "assets/images/skybox";

    initEntityFactory();
    registerComponents(entityFactory.getComponentFactory());
    registerComponentDisplayers();

    ui.init(gfxDevice);

    { // create camera
        static const float zNear = 1.f;
        static const float zFar = 64.f;
        static const float fovX = glm::radians(45.f);
        static const float aspectRatio = (float)params.screenWidth / (float)params.screenHeight;

        camera.setUseInverseDepth(true);
        camera.init(fovX, zNear, zFar, aspectRatio);
    }

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

void Game::customUpdate(float dt)
{
    ZoneScopedN("Update");

    auto& io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard) {
        handleInput(dt);
    }

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
    movementSystemUpdate(registry, dt);
    transformSystemUpdate(registry, dt);
    skeletonAnimationSystemUpdate(registry, dt);

    updateDevTools(dt);
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

void Game::customCleanup()
{
    registry.clear();

    gfxDevice.waitIdle();
    spriteRenderer.cleanup();
    renderer.cleanup();
    baseRenderer.cleanup();
}

void Game::setEntityTag(entt::handle e, const std::string& tag)
{
    auto& tc = e.get_or_emplace<TagComponent>();
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
bool shouldCastShadow(const entt::const_handle e)
{
    const auto& tag = eu::getTag(e);
    return !tag.starts_with("GroundTile");
}
}

void Game::customDraw()
{
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

        auto cmd = gfxDevice.beginFrame();
        renderer.draw(cmd, camera, sceneData);

        { // UI
            TracyVkZoneC(gfxDevice.getTracyVkCtx(), cmd, "UI", tracy::Color::Purple);
            vkutil::cmdBeginLabel(cmd, "UI");
            spriteRenderer.draw(cmd, renderer.getDrawImage());
            vkutil::cmdEndLabel(cmd);
        }

        gfxDevice.endFrame(cmd, renderer.getDrawImage());
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
        bool castShadow = shouldCastShadow({registry, e});
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
        renderer.drawSkinnedMesh(
            mc.meshes, sc.skinnedMeshes, tc.worldTransform, sc.skeletonAnimator.getJointMatrices());
#ifndef NDEBUG
        // 1. Not all meshes for the entity might be skinned
        // 2. Different meshes can have different joint matrices sets
        // But right now it's better to group meshes to not waste memory
        // It's also assumed that all skinned meshes have meshTransform[i] == I
        for (const auto& t : mc.meshTransforms) {
            assert(t.isIdentity());
        }
#endif
    }

    renderer.endDrawing();

    spriteRenderer.beginDrawing();
    ui.draw(spriteRenderer);
    spriteRenderer.endDrawing();
}

void Game::loadLevel(const std::filesystem::path& path)
{
    level.load(path);

    // load skybox
    if (level.hasSkybox()) {
        const auto skyboxImageId = gfxDevice.addImageToCache(
            graphics::loadCubemap(gfxDevice, skyboxDir / level.getSkyboxName()));
        renderer.setSkyboxImage(skyboxImageId);
    } else {
        // no skybox
        renderer.setSkyboxImage(NULL_IMAGE_ID);
    }

    loadScene(level.getSceneModelPath(), true);
}

void Game::loadScene(const std::filesystem::path& path, bool createEntities)
{
    const auto& scene = sceneCache.loadScene(baseRenderer, path);
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
        return entt::handle(registry, e);
    });

    entityFactory.setPostInitEntityFunc(
        [this](entt::handle e) { entityInitializer.initEntity(e); });

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
