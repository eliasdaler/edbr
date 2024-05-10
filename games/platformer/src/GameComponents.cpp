#include "Game.h"

#include "Components.h"

#include <edbr/ECS/Components/CollisionComponent2D.h>
#include <edbr/ECS/Components/MovementComponent.h>
#include <edbr/ECS/Components/NPCComponent.h>
#include <edbr/ECS/Components/SpriteAnimationComponent.h>
#include <edbr/ECS/Components/SpriteComponent.h>

void Game::registerComponents(ComponentFactory& cf)
{
    cf.registerComponentLoader(
        "sprite", [](entt::handle e, SpriteComponent& sc, const JsonDataLoader& loader) {
            loader.get("texture", sc.spritePath);
            loader.get("z", sc.z, 0);
        });

    cf.registerComponentLoader(
        "collision", [](entt::handle e, CollisionComponent2D& cc, const JsonDataLoader& loader) {
            loader.getIfExists("size", cc.size);
            loader.getIfExists("origin", cc.origin);
        });

    cf.registerComponentLoader(
        "animation",
        [this](entt::handle e, SpriteAnimationComponent& ac, const JsonDataLoader& loader) {
            loader.get("data", ac.animationsDataTag);

            auto it = animationsData.find(ac.animationsDataTag);
            if (it == animationsData.end()) {
                throw std::runtime_error(
                    fmt::format("animations data '{}' was not loaded", ac.animationsDataTag));
            }
            ac.animationsData = &it->second;

            loader.getIfExists("default_animation", ac.defaultAnimationName);
        });

    cf.registerComponentLoader(
        "movement", [](entt::handle e, MovementComponent& mc, const JsonDataLoader& loader) {
            loader.get("maxSpeed", mc.maxSpeed);
        });

    cf.registerComponent<SpawnerComponent>("spawner");

    cf.registerComponentLoader(
        "teleport", [](entt::handle e, TeleportComponent& tc, const JsonDataLoader& loader) {
            loader.getIfExists("to_level", tc.levelTag);
            loader.getIfExists("to", tc.spawnTag);
        });

    cf.registerComponentLoader(
        "character_controller",
        [](entt::handle e, CharacterControllerComponent& cc, const JsonDataLoader& loader) {});

    cf.registerComponentLoader(
        "interaction", [](entt::handle e, InteractComponent& ic, const JsonDataLoader& loader) {
            std::string interactTypeString;
            loader.getIfExists("type", interactTypeString);
            if (interactTypeString == "Examine") {
                ic.type = InteractComponent::Type::Examine;
            } else if (interactTypeString == "Talk") {
                ic.type = InteractComponent::Type::Talk;
            } else if (interactTypeString == "GoInside") {
                ic.type = InteractComponent::Type::GoInside;
            } else if (!interactTypeString.empty()) {
                throw std::runtime_error(
                    fmt::format("unknown interact type '{}'", interactTypeString));
            }
        });

    cf.registerComponentLoader(
        "npc", [](entt::handle e, NPCComponent& npcc, const JsonDataLoader& loader) {
            std::string nameTag;
            loader.getIfExists("name", nameTag);
            if (!nameTag.empty()) {
                npcc.name = LST{nameTag};
            }

            std::string defaultTextTag;
            loader.getIfExists("text", defaultTextTag);
            if (!defaultTextTag.empty()) {
                npcc.defaultText = LST{defaultTextTag};
            }
        });
}
