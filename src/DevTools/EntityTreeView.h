#pragma once

#include <glm/vec4.hpp>

#include <entt/entt.hpp>
#include <imgui.h>

class EntityTreeView {
public:
    void update(const entt::registry& registry, float dt);

    void setSelectedEntity(const entt::handle e) { selectedEntity = e; };
    entt::const_handle getSelectedEntity() const { return selectedEntity; }
    bool hasSelectedEntity() const { return selectedEntity.entity() != entt::null; }

private:
    entt::const_handle selectedEntity{};
    void updateEntityTreeUI(entt::const_handle e);

    bool displayEntityInView(entt::const_handle e, const std::string& label) const;
    glm::vec4 getDisplayColor(entt::const_handle e) const;

    bool displayStaticGeometry{false};
    bool displayTaggedStaticGeometry{true};
    bool displayLights{true};
    bool displayTriggers{true};
    bool displayColliders{false};

    ImGuiTextFilter filter;
};
