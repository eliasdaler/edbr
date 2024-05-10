#include "Game.h"

#include "Components.h"

#include <edbr/GameCommon/CommonComponentLoaders.h>
#include <edbr/GameCommon/CommonComponentLoaders2D.h>

void Game::registerComponents(ComponentFactory& cf)
{
    edbr::registerMovementComponentLoader(cf);
    edbr::registerCollisionComponent2DLoader(cf);
    edbr::registerSpriteComponentLoader(cf);
    edbr::registerSpriteAnimationComponentLoader(cf, animationsData);
    edbr::registerNPCComponentLoader(cf);

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
}
