#include <edbr/ECS/Systems/MovementSystem.h>

#include <edbr/ECS/Components/MovementComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>

#include <entt/entity/registry.hpp>

namespace
{
glm::vec3 getWorldPosition(const TransformComponent& tc)
{
    return glm::vec3{tc.worldTransform[3]};
}
} // end of anonymous namespace

namespace edbr::ecs
{
void movementSystemUpdate(entt::registry& registry, float dt)
{
    for (auto&& [e, tc, mc] : registry.view<TransformComponent, MovementComponent>().each()) {
        mc.prevFramePosition = ::getWorldPosition(tc);
        const auto newPos = tc.transform.getPosition() + mc.kinematicVelocity * dt;
        tc.transform.setPosition(newPos);

        if (mc.rotationTime != 0.f) {
            mc.rotationProgress += dt;
            if (mc.rotationProgress >= mc.rotationTime) {
                tc.transform.setHeading(mc.targetHeading);
                mc.rotationProgress = mc.rotationTime;
                mc.rotationTime = 0.f;
                continue;
            }

            const auto newHeading = glm::
                slerp(mc.startHeading, mc.targetHeading, mc.rotationProgress / mc.rotationTime);
            tc.transform.setHeading(newHeading);
        }
    }
}

void movementSystemPostPhysicsUpdate(entt::registry& registry, float dt)
{
    for (auto&& [e, tc, mc] : registry.view<TransformComponent, MovementComponent>().each()) {
        auto newPos = ::getWorldPosition(tc);
        mc.effectiveVelocity = (newPos - mc.prevFramePosition) / dt;
    }
}

} // end of namespace edbr::ecs
