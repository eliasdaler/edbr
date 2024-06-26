#include <edbr/GameCommon/CommonComponentDisplayers.h>

#include <edbr/DevTools/EntityInfoDisplayer.h>
#include <edbr/DevTools/ImGuiPropertyTable.h>

#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/ECS/Components/MovementComponent.h>
#include <edbr/ECS/Components/NPCComponent.h>
#include <edbr/ECS/Components/TagComponent.h>

using namespace devtools;

namespace edbr
{
void registerMetaInfoComponentDisplayer(EntityInfoDisplayer& eid)
{
    eid.registerDisplayer("Meta", [](entt::handle e, const MetaInfoComponent& tc) {
        BeginPropertyTable();
        {
            DisplayProperty("Prefab", tc.prefabName);
        }
        EndPropertyTable();
    });
}

void registerTagComponentDisplayer(EntityInfoDisplayer& eid)
{
    eid.registerDisplayer("Tag", [](entt::handle e, const TagComponent& tc) {
        BeginPropertyTable();
        DisplayProperty("Tag", tc.tag);
        EndPropertyTable();
    });
}

void registerMovementComponentDisplayer(EntityInfoDisplayer& eid)
{
    eid.registerDisplayer("Movement", [](entt::handle e, const MovementComponent& mc) {
        BeginPropertyTable();
        {
            DisplayProperty("MaxSpeed", mc.maxSpeed);
            DisplayProperty("Velocity (kinematic)", mc.kinematicVelocity);
            // don't display for now: too noisy
            // DisplayProperty("Velocity (effective)", mc.effectiveVelocity);
            if (mc.rotationTime != 0.f) {
                DisplayProperty("Start heading", mc.startHeading);
                DisplayProperty("Target heading", mc.targetHeading);
                DisplayProperty("Rotation progress", mc.rotationProgress);
                DisplayProperty("Rotation time", mc.rotationTime);
            }
        }
        EndPropertyTable();
    });
}

void registerNPCComponentDisplayer(EntityInfoDisplayer& eid)
{
    eid.registerDisplayer("NPC", [](entt::handle e, const NPCComponent& npcc) {
        BeginPropertyTable();
        {
            if (!npcc.name.tag.empty()) {
                DisplayProperty("Name", npcc.name.tag);
            }
        }
        EndPropertyTable();
    });
}

}
