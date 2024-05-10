#include <edbr/GameCommon/EntityUtil.h>

#include <edbr/ECS/Components/PersistentComponent.h>
#include <edbr/ECS/Components/TagComponent.h>

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

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

const std::string& getTag(entt::const_handle e)
{
    static const std::string emptyString{};
    if (auto tcPtr = e.try_get<TagComponent>(); tcPtr) {
        return tcPtr->tag;
    }
    return emptyString;
}

void makePersistent(entt::handle e)
{
    e.emplace<PersistentComponent>();
}

void makeNonPersistent(entt::handle e)
{
    e.erase<PersistentComponent>();
}

}
