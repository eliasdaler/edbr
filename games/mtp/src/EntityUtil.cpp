#include "EntityUtil.h"

#include "Components.h"

#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/ECS/Components/MetaInfoComponent.h>

namespace entityutil
{

void addChild(entt::handle parent, entt::handle child)
{
    auto& parentHC = parent.get<HierarchyComponent>();

    // check that the same child wasn't added twice
    const auto it = std::find(parentHC.children.begin(), parentHC.children.end(), child);
    if (it != parentHC.children.end()) {
        assert(false && "child was already added");
        return;
    }

    auto& childHC = child.get<HierarchyComponent>();
    if (childHC.hasParent()) { // remove from previous parent
        auto& prevParentHC = childHC.parent.get<HierarchyComponent>();
        std::erase(prevParentHC.children, child);
    }

    // establish child-parent relationship
    childHC.parent = parent;
    parentHC.children.push_back(child);
}

glm::vec3 getWorldPosition(entt::handle e)
{
    return glm::vec3(e.get<TransformComponent>().worldTransform[3]);
}

glm::vec3 getLocalPosition(entt::handle e)
{
    return e.get<TransformComponent>().transform.getPosition();
}

void setPosition(entt::handle e, const glm::vec3& pos)
{
    assert(!e.get<HierarchyComponent>().hasParent());
    auto& tc = e.get<TransformComponent>();
    tc.transform.setPosition(pos);
    tc.worldTransform = tc.transform.asMatrix();
}

void setRotation(entt::handle e, const glm::quat& rotation)
{
    assert(!e.get<HierarchyComponent>().hasParent());
    auto& tc = e.get<TransformComponent>();
    tc.transform.setHeading(rotation);
    tc.worldTransform = tc.transform.asMatrix();
}

void setAnimation(entt::handle e, const std::string& name)
{
    auto scPtr = e.try_get<SkeletonComponent>();
    if (!scPtr) {
        return;
    }
    auto& sc = *scPtr;
    sc.skeletonAnimator.setAnimation(sc.skeleton, sc.animations.at(name));
}

entt::handle findEntityBySceneNodeName(entt::registry& registry, const std::string& name)
{
    for (auto&& [e, mic] : registry.view<MetaInfoComponent>().each()) {
        if (mic.sceneNodeName == name) {
            return {registry, e};
        }
    }
    return {};
}

entt::handle findPlayerSpawnByName(entt::registry& registry, const std::string& name)
{
    return findEntityByName<PlayerSpawnComponent>(registry, name);
}

entt::handle findCameraByName(entt::registry& registry, const std::string& name)
{
    return findEntityByName<CameraComponent>(registry, name);
}

entt::handle getPlayerEntity(entt::registry& registry)
{
    for (const auto& e : registry.view<PlayerComponent>()) {
        return {registry, e};
    }
    fmt::println("Player was not found");
    return {};
}

void spawnPlayer(entt::registry& registry, const std::string& spawnName)
{
    auto player = getPlayerEntity(registry);
    if (player.entity() == entt::null) {
        return;
    }

    const auto spawn = findPlayerSpawnByName(registry, spawnName);
    if (spawn.entity() == entt::null) {
        return;
    }

    auto& playerTC = player.get<TransformComponent>();
    const auto& spawnTC = spawn.get<TransformComponent>();

    // copy position and heading (don't copy scale)
    playerTC.transform.setPosition(spawnTC.transform.getPosition());
    playerTC.transform.setHeading(spawnTC.transform.getHeading());
    assert(!player.get<HierarchyComponent>().hasParent());
    playerTC.worldTransform = playerTC.transform.asMatrix();
}

const std::string& getTag(entt::const_handle e)
{
    static const std::string emptyString{};
    if (auto tcPtr = e.try_get<TagComponent>(); tcPtr && !tcPtr->getTag().empty()) {
        return tcPtr->getTag();
    }
    return emptyString;
}

}
