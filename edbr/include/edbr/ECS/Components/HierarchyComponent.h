#pragma once

#include <vector>

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

struct HierarchyComponent {
    entt::handle parent{};
    std::vector<entt::handle> children;

    bool hasParent() const
    {
        // would ideally use parent.valid() instead, but enTT segfaults
        // if no registry is set for the handle
        return (bool)parent;
    }
};
