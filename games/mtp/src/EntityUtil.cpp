#include "EntityUtil.h"

#include "Components.h"
#include "Events.h"

#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/ECS/Components/PersistentComponent.h>
#include <edbr/Event/EventManager.h>

namespace entityutil
{

EventManager* eventManager{nullptr};

void setEventManager(EventManager& em)
{
    eventManager = &em;
}

void makePersistent(entt::handle e)
{
    e.emplace<PersistentComponent>();
}

void makeNonPersistent(entt::handle e)
{
    e.erase<PersistentComponent>();
}

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
    assert(!e.get<HierarchyComponent>().hasParent() && "can't set position on parented entity");
    assert(!e.all_of<PhysicsComponent>() && "call teleportEntity instead");

    auto& tc = e.get<TransformComponent>();
    tc.transform.setPosition(pos);
    tc.worldTransform = tc.transform.asMatrix();
}

void teleportEntity(entt::handle e, const glm::vec3& pos)
{
    assert(!e.get<HierarchyComponent>().hasParent() && "can't set position on parented entity");
    assert(e.all_of<PhysicsComponent>() && "call setPosition instead");
    auto& tc = e.get<TransformComponent>();
    tc.transform.setPosition(pos);
    tc.worldTransform = tc.transform.asMatrix();

    assert(eventManager);
    EntityTeleportedEvent event;
    event.entity = e;
    eventManager->triggerEvent(event);
}

void setRotation(entt::handle e, const glm::quat& rotation)
{
    assert(!e.get<HierarchyComponent>().hasParent());
    auto& tc = e.get<TransformComponent>();
    tc.transform.setHeading(rotation);
    tc.worldTransform = tc.transform.asMatrix();
}

void rotateSmoothlyTo(entt::handle e, const glm::quat& targetHeading, float rotationTime)
{
    const auto& tc = e.get<TransformComponent>();
    auto& mc = e.get<MovementComponent>();
    mc.startHeading = tc.transform.getHeading();
    mc.targetHeading = targetHeading;
    if (glm::dot(mc.startHeading, targetHeading) < 0) {
        mc.targetHeading = -mc.targetHeading; // this gives us the shortest rotation
    }
    mc.rotationTime = rotationTime;
    mc.rotationProgress = 0.f;
}

void stopRotation(entt::handle e)
{
    auto& mc = e.get<MovementComponent>();
    mc.rotationProgress = mc.rotationTime;
    mc.rotationTime = 0.f;
}

void setAnimation(entt::handle e, const std::string& name)
{
    auto scPtr = e.try_get<SkeletonComponent>();
    if (!scPtr) {
        return;
    }
    auto& sc = *scPtr;
    assert(sc.animations);
    sc.skeletonAnimator.setAnimation(sc.skeleton, sc.animations->at(name));
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
    return {};
}

bool playerExists(entt::registry& registry)
{
    return getPlayerEntity(registry).entity() != entt::null;
}

void spawnPlayer(entt::registry& registry, const std::string& spawnName)
{
    auto player = getPlayerEntity(registry);
    if (player.entity() == entt::null) {
        return;
    }

    const auto spawn = findPlayerSpawnByName(registry, spawnName);
    if (spawn.entity() == entt::null) {
        fmt::println("[error]: spawn '{}' was not found", spawnName);
        return;
    }

    auto& playerTC = player.get<TransformComponent>();
    const auto& spawnTC = spawn.get<TransformComponent>();

    // copy heading (don't copy scale)
    playerTC.transform.setHeading(spawnTC.transform.getHeading());

    // teleport
    teleportEntity(player, spawnTC.transform.getPosition());
}

const std::string& getTag(entt::const_handle e)
{
    static const std::string emptyString{};
    if (auto tcPtr = e.try_get<TagComponent>(); tcPtr && !tcPtr->getTag().empty()) {
        return tcPtr->getTag();
    }
    return emptyString;
}

const std::string& getMetaName(entt::const_handle e)
{
    // try tag first
    auto& tag = getTag(e);
    if (!tag.empty()) {
        return tag;
    }

    // then, try name component
    if (auto ncPtr = e.try_get<NameComponent>(); ncPtr && !ncPtr->name.empty()) {
        return ncPtr->name;
    }

    // if everything failed - use scene node name
    return e.get<MetaInfoComponent>().sceneNodeName;
}

}
