#include <edbr/GameCommon/CommonComponentDisplayers2D.h>

#include <edbr/DevTools/EntityInfoDisplayer.h>
#include <edbr/DevTools/ImGuiPropertyTable.h>

#include <edbr/ECS/Components/CollisionComponent2D.h>
#include <edbr/ECS/Components/SpriteAnimationComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>

#include <edbr/GameCommon/EntityUtil2D.h>

using namespace devtools;

namespace edbr
{
void registerTransformComponentDisplayer2D(EntityInfoDisplayer& eid)
{
    eid.registerDisplayer("Transform", [](entt::handle e, const TransformComponent& tc) {
        BeginPropertyTable();
        {
            DisplayProperty("Position2D", entityutil::getWorldPosition2D(e));
            DisplayProperty("Heading2D", entityutil::getHeading2D(e));
        }
        EndPropertyTable();
    });
}

void registerCollisionComponent2DDisplayer(EntityInfoDisplayer& eid)
{
    eid.registerDisplayer("Collision", [](entt::handle e, const CollisionComponent2D& cc) {
        BeginPropertyTable();
        {
            DisplayProperty("Size", cc.size);
            DisplayProperty("Origin", cc.origin);
        }
        EndPropertyTable();
    });
}

void registerSpriteAnimationComponentDisplayer(EntityInfoDisplayer& eid)
{
    eid.registerDisplayer("Animation", [](entt::handle e, const SpriteAnimationComponent& ac) {
        BeginPropertyTable();
        {
            DisplayProperty("Animation", ac.animator.getAnimationName());
            DisplayProperty("Frame", ac.animator.getCurrentFrame());
            DisplayProperty("Progress", ac.animator.getProgress());
            DisplayProperty("Anim data", ac.animationsDataTag);
        }
        EndPropertyTable();
    });
}

}
