#include "Game.h"

#include <edbr/DevTools/ImGuiPropertyTable.h>

#include "Components.h"

#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/ECS/Components/NameComponent.h>

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

#include <edbr/Util/JoltUtil.h>

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
            DisplayProperty("Velocity (kinematic)", mc.kinematicVelocity);
            DisplayProperty("MaxSpeed", mc.maxSpeed);
            // don't display for now: too noisy
            // DisplayProperty("Velocity (effective)", mc.effectiveVelocity);
            DisplayProperty("Start heading", mc.startHeading);
            DisplayProperty("Target heading", mc.targetHeading);
            DisplayProperty("Rotation progress", mc.rotationProgress);
            DisplayProperty("Rotation time", mc.rotationTime);
        }
        EndPropertyTable();
    });

    eid.registerDisplayer("Physics", [this](entt::const_handle e, const PhysicsComponent& pc) {
        BeginPropertyTable();
        {
            DisplayProperty("bodyId", pc.bodyId.GetIndex());

            physicsSystem->doForBody(pc.bodyId, [](const JPH::Body& body) {
                DisplayProperty("Position", util::joltToGLM(body.GetPosition()));
                DisplayProperty("Center of mass", util::joltToGLM(body.GetCenterOfMassPosition()));
                DisplayProperty("Rotation", util::joltToGLM(body.GetRotation()));
                DisplayProperty("Linear velocity", util::joltToGLM(body.GetLinearVelocity()));
                DisplayProperty("Angular velocity", util::joltToGLM(body.GetAngularVelocity()));
                DisplayProperty("Object layer", body.GetObjectLayer());
                DisplayProperty("Broadphase layer", body.GetBroadPhaseLayer().GetValue());
                const auto motionTypeStr = [](JPH::EMotionType t) {
                    switch (t) {
                    case JPH::EMotionType::Dynamic:
                        return "Dynamic";
                    case JPH::EMotionType::Kinematic:
                        return "Kinematic";
                    case JPH::EMotionType::Static:
                        return "Static";
                    }
                    return "Unknown";
                }(body.GetMotionType());
                DisplayProperty("Motion type", motionTypeStr);
                DisplayProperty("Active", body.IsActive());
                DisplayProperty("Sensor", body.IsSensor());
                if (!body.IsStatic()) {
                    const auto& motionProps = *body.GetMotionProperties();
                    DisplayProperty("Gravity factor", motionProps.GetGravityFactor());
                }
            });
        }
        EndPropertyTable();
    });

    eid.registerDisplayer("Mesh", [](entt::const_handle e, const MeshComponent& mc) {
        BeginPropertyTable();
        {
            DisplayProperty("meshPath", mc.meshPath.string());
            for (const auto& id : mc.meshes) {
                DisplayProperty("mesh", id);
            }
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
            DisplayProperty("Frame", animator.getCurrentFrame());
            DisplayProperty("Looped", animator.getAnimation()->looped);
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
            DisplayProperty("Type", lightTypeString);

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
    eid.registerEmptyDisplayer<ColliderComponent>("Collider");

    eid.registerEmptyDisplayer<CameraComponent>("Camera", [this](entt::const_handle e) {
        if (ImGui::Button("Make current")) {
            setCurrentCamera(e);
        }
    });

    eid.registerDisplayer(
        "AnimationEventSound", [](entt::const_handle e, const AnimationEventSoundComponent& sc) {
            BeginPropertyTable();
            {
                for (const auto& [event, sound] : sc.eventSounds) {
                    DisplayProperty(event.c_str(), sound);
                }
            }
            EndPropertyTable();
        });
}