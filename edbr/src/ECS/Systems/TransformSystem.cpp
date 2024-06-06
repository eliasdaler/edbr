#include <edbr/ECS/Systems/TransformSystem.h>

#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>

namespace
{
void updateEntityTransforms(
    entt::registry& registry,
    entt::entity e,
    const glm::mat4& parentWorldTransform)
{
    auto [tc, hc] = registry.get<TransformComponent, const HierarchyComponent>(e);
    if (!hc.hasParent()) {
        tc.worldTransform = tc.transform.asMatrix();
    } else {
        const auto prevTransform = tc.worldTransform;
        tc.worldTransform = parentWorldTransform * tc.transform.asMatrix();
        if (tc.worldTransform == prevTransform) {
            return;
        }
    }

    for (const auto& child : hc.children) {
        updateEntityTransforms(registry, child, tc.worldTransform);
    }
}
} // end of anonymous namespace

namespace edbr::ecs
{
void transformSystemUpdate(entt::registry& registry, float dt)
{
    static const auto I = glm::mat4{1.f};
    for (auto&& [e, hc] : registry.view<HierarchyComponent>().each()) {
        if (!hc.hasParent()) { // start from root nodes
            updateEntityTransforms(registry, e, I);
        }
    }
}
}
