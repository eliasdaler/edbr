#include "Game.h"

#include <edbr/DevTools/ImGuiPropertyTable.h>

#include <edbr/ECS/Components/NameComponent.h>
#include <edbr/ECS/Components/PersistentComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>

#include <edbr/GameCommon/CommonComponentDisplayers.h>
#include <edbr/GameCommon/CommonComponentDisplayers2D.h>

#include "Components.h"

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

void Game::registerComponentDisplayers()
{
    using namespace devtools;

    auto& eid = entityInfoDisplayer;

    edbr::registerMetaInfoComponentDisplayer(eid);
    edbr::registerTagComponentDisplayer(eid);

    eid.registerDisplayer("Name", [](entt::handle e, const NameComponent& nc) {
        BeginPropertyTable();
        if (!nc.name.empty()) {
            DisplayProperty("Name", nc.name);
        }
        EndPropertyTable();
    });

    edbr::registerTransformComponentDisplayer2D(eid);
    edbr::registerMovementComponentDisplayer(eid);
    edbr::registerCollisionComponent2DDisplayer(eid);
    edbr::registerSpriteAnimationComponentDisplayer(eid);

    eid.registerDisplayer(
        "CharacterController", [](entt::handle e, const CharacterControllerComponent& cc) {
            BeginPropertyTable();
            {
                DisplayProperty("Gravity", cc.gravity);
                DisplayProperty("WasOnGround", cc.wasOnGround);
                DisplayProperty("IsOnGround", cc.isOnGround);
                DisplayProperty("WantJump", cc.wantJump);
            }
            EndPropertyTable();
        });

    eid.registerEmptyDisplayer<SpawnerComponent>("Spawner");

    eid.registerDisplayer("Teleport", [](entt::handle e, const TeleportComponent& tc) {
        BeginPropertyTable();
        {
            DisplayProperty("Level", tc.levelTag);
            DisplayProperty("Spawn", tc.spawnTag);
        }
        EndPropertyTable();
    });

    eid.registerDisplayer("Interact", [](entt::handle e, const InteractComponent& ic) {
        BeginPropertyTable();
        {
            const auto interactTypeString = [](InteractComponent::Type t) {
                switch (t) {
                case InteractComponent::Type::None:
                    return "None";
                case InteractComponent::Type::Examine:
                    return "Examine";
                case InteractComponent::Type::Talk:
                    return "Talk";
                case InteractComponent::Type::GoInside:
                    return "GoInside";
                default:
                    return "???";
                }
            }(ic.type);

            DisplayProperty("Type", interactTypeString);
        }
        EndPropertyTable();
    });

    edbr::registerNPCComponentDisplayer(eid);
}
