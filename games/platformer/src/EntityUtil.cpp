#include "EntityUtil.h"

#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/ECS/Components/MovementComponent.h>
#include <edbr/ECS/Components/PersistentComponent.h>
#include <edbr/ECS/Components/TransformComponent.h>

#include "Components.h"

#include <fmt/printf.h>

namespace entityutil
{

entt::handle getEntityByTag(entt::registry& registry, const std::string& tag)
{
    for (const auto&& [e, tc] : registry.view<TagComponent>().each()) {
        if (tc.tag == tag) {
            return {registry, e};
        }
    }
    return {};
}

void setTag(entt::handle e, const std::string& tag)
{
    assert(!tag.empty() && "tag can't be empty");
    // ensure that tags are unique
    assert(
        getEntityByTag(*e.registry(), tag).entity() == entt::null &&
        "tag already assigned to another entity");

    e.emplace<TagComponent>(TagComponent{.tag = tag});
}

void setWorldPosition2D(entt::handle e, const glm::vec2& pos)
{
    assert(
        !e.get<HierarchyComponent>().hasParent() &&
        "can't set position on an entity with a parent");
    auto& tc = e.get<TransformComponent>();
    tc.transform.setPosition(glm::vec3{pos, tc.transform.getPosition().z});
    tc.worldTransform = tc.transform.asMatrix();
}

glm::vec2 getWorldPosition2D(entt::const_handle e)
{
    return glm::vec2(e.get<TransformComponent>().worldTransform[3]);
}

void setHeading2D(entt::handle e, const glm::vec2& u)
{
    const auto angle = std::atan2(u.y, u.x);
    const auto heading = glm::angleAxis(angle, glm::vec3{0.f, 0.f, 1.f});
    auto& tc = e.get<TransformComponent>();
    tc.transform.setHeading(heading);
}

glm::vec2 getHeading2D(entt::const_handle e)
{
    const auto& tc = e.get<TransformComponent>();
    return glm::vec2{tc.transform.getHeading() * glm::vec3{1.f, 0.f, 0.f}};
}

math::FloatRect getAABB(entt::const_handle e)
{
    auto& cc = e.get<CollisionComponent>();
    auto origin = cc.origin;
    if (origin == glm::vec2{}) {
        origin = {-cc.size.x / 2.f, -cc.size.y};
    }
    const auto pos = getWorldPosition2D(e);
    const auto tl = pos + origin;
    return {tl, cc.size};
}

void makePersistent(entt::handle e)
{
    e.emplace<PersistentComponent>();
}

void makeNonPersistent(entt::handle e)
{
    e.erase<PersistentComponent>();
}

math::FloatRect getSpriteWorldRect(entt::const_handle e)
{
    // this assumes that entity/sprite doesn't have scale
    const auto& gc = e.get<SpriteComponent>();
    const auto ss = glm::abs(gc.sprite.uv1 - gc.sprite.uv0) * gc.sprite.textureSize;
    const auto pos2D = getWorldPosition2D(e);
    return math::FloatRect(pos2D - gc.sprite.pivot * ss, ss);
}

void setSpriteAnimation(entt::handle e, const std::string& animName)
{
    auto& ac = e.get<SpriteAnimationComponent>();
    if (ac.animator.getAnimationName() == animName) {
        return;
    }
    if (!ac.animationsData->hasAnimation(animName)) {
        fmt::println(
            "animation data {} doesn't have animation named {}", ac.animationsDataTag, animName);
        return;
    }

    auto& sc = e.get<SpriteComponent>();

    ac.animator.setAnimation(ac.animationsData->getAnimation(animName), animName);
    ac.animator.animate(sc.sprite, ac.animationsData->getSpriteSheet());
}

entt::handle getPlayerEntity(entt::registry& registry)
{
    for (const auto& e : registry.view<PlayerComponent>()) {
        return {registry, e};
    }
    return {};
}

}
