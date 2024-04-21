#pragma once

#include <edbr/Graphics/Color.h>

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

#include <imgui.h>

class EntityTreeView {
public:
    virtual ~EntityTreeView() = default;

    void update(entt::registry& registry, float dt);

    void setSelectedEntity(const entt::handle e) { selectedEntity = e; };
    entt::handle getSelectedEntity() const { return selectedEntity; }
    void deselectedEntity() { setSelectedEntity({}); }
    bool hasSelectedEntity() const { return selectedEntity.entity() != entt::null; }

    virtual void displayExtraFilters(){};
    virtual bool displayEntityInView(entt::const_handle e, const std::string& label) const
    {
        return true;
    }

    virtual const std::string& getEntityDisplayName(entt::const_handle e) const;
    virtual RGBColor getDisplayColor(entt::const_handle e) const { return RGBColor{255, 255, 255}; }

private:
    entt::handle selectedEntity{};
    void updateEntityTreeUI(entt::handle e);

    ImGuiTextFilter filter;
};
