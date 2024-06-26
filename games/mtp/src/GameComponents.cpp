#include "Game.h"

#include <edbr/ECS/Components/SceneComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>
#include <edbr/GameCommon/CommonComponentLoaders.h>

namespace
{
void loadPhysicsComponent(entt::handle e, PhysicsComponent& pc, const JsonDataLoader& loader);
}

void Game::registerComponents(ComponentFactory& cf)
{
    edbr::registerMovementComponentLoader(cf);
    edbr::registerNPCComponentLoader(cf);

    cf.registerComponent<TriggerComponent>("trigger");
    cf.registerComponent<CameraComponent>("camera");
    cf.registerComponent<PlayerSpawnComponent>("player_spawn");
    cf.registerComponent<ColliderComponent>("collider");

    cf.registerComponentLoader(
        "scene", [](entt::handle e, SceneComponent& sc, const JsonDataLoader& loader) {
            loader.getIfExists("scene", sc.sceneName);
            loader.getIfExists("node", sc.sceneNodeName);
        });

    cf.registerComponentLoader(
        "mesh", [](entt::handle e, MeshComponent& mc, const JsonDataLoader& loader) {
            loader.getIfExists("castShadow", mc.castShadow);
        });

    cf.registerComponentLoader("physics", loadPhysicsComponent);

    cf.registerComponentLoader(
        "interact", [](entt::handle e, InteractComponent& ic, const JsonDataLoader& loader) {
            // type
            std::string type;
            loader.getIfExists("type", type);
            if (type == "examine") {
                ic.type = InteractComponent::Type::Interact;
            } else if (type == "talk") {
                ic.type = InteractComponent::Type::Talk;
            } else if (!type.empty()) {
                fmt::println("[error] unknown interact component type '{}'", type);
            }
        });

    cf.registerComponentLoader(
        "animation_event_sound",
        [](entt::handle e, AnimationEventSoundComponent& sc, const JsonDataLoader& loader) {
            sc.eventSounds = loader.getLoader("events").getKeyValueMapString();
        });

    cf.registerComponentLoader(
        "face", [](entt::handle e, FaceComponent& fc, const JsonDataLoader& loader) {
            loader.get("facesTexturesDir", fc.facesTexturesDir);
            fc.facesFilenames = loader.getLoader("faces").getKeyValueMapString();
            loader.get("defaultFace", fc.defaultFace);
        });

    cf.registerComponentLoader(
        "blink", [](entt::handle e, BlinkComponent& bc, const JsonDataLoader& loader) {
            loader.get("blinkPeriod", bc.blinkPeriod);
            loader.get("blinkHold", bc.blinkHold);
            bc.faces = loader.getLoader("faces").getKeyValueMapString();
        });
}

namespace
{
void loadPhysicsComponent(entt::handle e, PhysicsComponent& pc, const JsonDataLoader& loader)
{
    // type
    std::string type;
    loader.getIfExists("type", type);
    if (type == "static") {
        pc.type = PhysicsComponent::Type::Static;
    } else if (type == "dynamic") {
        pc.type = PhysicsComponent::Type::Dynamic;
    } else if (type == "kinematic") {
        pc.type = PhysicsComponent::Type::Kinematic;
    } else if (!type.empty()) {
        fmt::println("[error] unknown physics component type '{}'", type);
    }

    // origin type
    std::string originType;
    loader.getIfExists("originType", originType);
    if (originType == "bottom_plane") {
        pc.originType = PhysicsComponent::OriginType::BottomPlane;
    } else if (originType == "center") {
        pc.originType = PhysicsComponent::OriginType::Center;
    } else if (!originType.empty()) {
        fmt::println("[error] unknown physics component origin type '{}'", type);
    }

    // sensor
    loader.getIfExists("sensor", pc.sensor);

    // body type + body params
    std::string bodyType;
    loader.getIfExists("bodyType", bodyType);
    if (bodyType == "aabb") {
        pc.bodyType = PhysicsComponent::BodyType::AABB;
        pc.bodyParams = PhysicsComponent::AABBParams{};
    } else if (bodyType == "capsule") {
        pc.bodyType = PhysicsComponent::BodyType::Capsule;

        const auto bodyLoader = loader.getLoader("bodyParams");
        auto params = PhysicsComponent::CapsuleParams{};
        bodyLoader.get("halfHeight", params.halfHeight);
        bodyLoader.get("radius", params.radius);

        pc.bodyParams = params;
    } else if (bodyType == "mesh") {
        pc.bodyType = PhysicsComponent::BodyType::TriangleMesh;
    } else if (bodyType == "virtual_character") {
        pc.bodyType = PhysicsComponent::BodyType::VirtualCharacter;

        VirtualCharacterParams characterParams{};
        characterParams.loadFromJson(loader.getLoader("bodyParams"));
        pc.bodyParams = characterParams;

    } else if (!bodyType.empty()) {
        // TODO: load other body types from JSON
        fmt::println("[error] unknown physics component body type '{}'", bodyType);
    }
}
}
