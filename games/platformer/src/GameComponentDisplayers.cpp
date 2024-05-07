#include "Game.h"

#include <edbr/DevTools/ImGuiPropertyTable.h>

#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/ECS/Components/MovementComponent.h>
#include <edbr/ECS/Components/NameComponent.h>
#include <edbr/ECS/Components/PersistentComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>

#include "Components.h"
#include "EntityUtil.h"

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

void Game::registerComponentDisplayers()
{
    using namespace devtools;

    auto& eid = entityInfoDisplayer;

    eid.registerDisplayer("Meta", [](entt::const_handle e, const MetaInfoComponent& tc) {
        BeginPropertyTable();
        {
            DisplayProperty("Prefab", tc.prefabName);
        }
        EndPropertyTable();
    });

    eid.registerDisplayer("Tag", [](entt::const_handle e, const TagComponent& tc) {
        BeginPropertyTable();
        DisplayProperty("Tag", tc.tag);
        EndPropertyTable();
    });

    eid.registerDisplayer("Name", [](entt::const_handle e, const NameComponent& nc) {
        BeginPropertyTable();
        if (!nc.name.empty()) {
            DisplayProperty("Name", nc.name);
        }
        EndPropertyTable();
    });

    eid.registerDisplayer("Transform", [](entt::const_handle e, const TransformComponent& tc) {
        BeginPropertyTable();
        {
            DisplayProperty("Position2D", entityutil::getWorldPosition2D(e));
            DisplayProperty("Heading2D", entityutil::getHeading2D(e));
        }
        EndPropertyTable();
    });

    eid.registerDisplayer("Movement", [](entt::const_handle e, const MovementComponent& mc) {
        BeginPropertyTable();
        {
            DisplayProperty("MaxSpeed", mc.maxSpeed);
            DisplayProperty("Velocity (kin)", mc.kinematicVelocity);
            DisplayProperty("Velocity (eff)", mc.effectiveVelocity);
            if (mc.rotationTime != 0.f) {
                DisplayProperty("Start heading", mc.startHeading);
                DisplayProperty("Target heading", mc.targetHeading);
                DisplayProperty("Rotation progress", mc.rotationProgress);
                DisplayProperty("Rotation time", mc.rotationTime);
            }
        }
        EndPropertyTable();
    });

    eid.registerDisplayer("Collision", [](entt::const_handle e, const CollisionComponent& cc) {
        BeginPropertyTable();
        {
            DisplayProperty("Size", cc.size);
            DisplayProperty("Origin", cc.origin);
        }
        EndPropertyTable();
    });

    eid.registerDisplayer(
        "CharacterController", [](entt::const_handle e, const CharacterControllerComponent& cc) {
            BeginPropertyTable();
            {
                DisplayProperty("Gravity", cc.gravity);
                DisplayProperty("WasOnGround", cc.wasOnGround);
                DisplayProperty("IsOnGround", cc.isOnGround);
                DisplayProperty("WantJump", cc.wantJump);
            }
            EndPropertyTable();
        });

    eid.registerDisplayer(
        "Animation", [](entt::const_handle e, const SpriteAnimationComponent& ac) {
            BeginPropertyTable();
            {
                DisplayProperty("Animation", ac.animator.getAnimationName());
                DisplayProperty("Frame", ac.animator.getCurrentFrame());
                DisplayProperty("Progress", ac.animator.getProgress());
                DisplayProperty("Anim data", ac.animationsDataTag);
            }
            EndPropertyTable();
        });

    eid.registerEmptyDisplayer<SpawnerComponent>("Spawner");

    eid.registerDisplayer("Teleport", [](entt::const_handle e, const TeleportComponent& tc) {
        BeginPropertyTable();
        {
            DisplayProperty("Level", tc.levelTag);
            DisplayProperty("Spawn", tc.spawnTag);
        }
        EndPropertyTable();
    });

    eid.registerDisplayer("Interact", [](entt::const_handle e, const InteractComponent& ic) {
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
}
