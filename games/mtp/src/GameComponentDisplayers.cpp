#include "Game.h"

#include <edbr/DevTools/ImGuiPropertyTable.h>

#include "Components.h"

#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/ECS/Components/NameComponent.h>

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
            if (!tc.sceneNodeName.empty()) {
                DisplayProperty("glTF node name", tc.sceneNodeName);
            }
        }
        EndPropertyTable();
    });

    eid.registerDisplayer("Tag", [](entt::const_handle e, const TagComponent& tc) {
        BeginPropertyTable();
        if (!tc.getTag().empty()) {
            DisplayProperty("Tag", tc.getTag());
        }
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
            DisplayProperty("Position", tc.transform.getPosition());
            DisplayProperty("Heading", tc.transform.getHeading());
            DisplayProperty("Scale", tc.transform.getScale());
        }
        EndPropertyTable();
    });

    eid.registerDisplayer("Movement", [](entt::const_handle e, const MovementComponent& mc) {
        BeginPropertyTable();
        {
            DisplayProperty("MaxSpeed", mc.maxSpeed);
            DisplayProperty("Velocity", mc.velocity);
        }
        EndPropertyTable();
    });

    eid.registerDisplayer("Skeleton", [](entt::const_handle e, const SkeletonComponent& sc) {
        const auto& animator = sc.skeletonAnimator;
        BeginPropertyTable();
        {
            DisplayProperty("Animation", animator.getCurrentAnimationName());
            DisplayProperty("Anim length", animator.getAnimation()->duration);
            DisplayProperty("Progress", animator.getProgress());
        }
        EndPropertyTable();
    });

    eid.registerDisplayer("Light", [](entt::const_handle e, const LightComponent& lc) {
        const auto& light = lc.light;

        const auto lightTypeString = [](LightType lt) {
            switch (lt) {
            case LightType::Directional:
                return "Directional";
            case LightType::Point:
                return "Point";
            case LightType::Spot:
                return "Spot";
            default:
                return "";
            }
        }(light.type);

        BeginPropertyTable();
        {
            DisplayProperty("Type", std::string{lightTypeString});

            DisplayColorProperty("Color", light.color);
            DisplayProperty("Range", light.range);
            DisplayProperty("Intensity", light.intensity);
            if (light.type == LightType::Spot) {
                DisplayProperty("Scale offset", light.scaleOffset);
            }
            DisplayProperty("Cast shadow", light.castShadow);
        }
        EndPropertyTable();
    });

    eid.registerEmptyDisplayer<TriggerComponent>("Trigger");
    eid.registerEmptyDisplayer<PlayerSpawnComponent>("PlayerSpawn");
    eid.registerEmptyDisplayer<PlayerComponent>("Player");

    eid.registerEmptyDisplayer<CameraComponent>("Camera", [this](entt::const_handle e) {
        if (ImGui::Button("Make current")) {
            setCurrentCamera(e);
        }
    });
}
