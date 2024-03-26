#pragma once

#include <edbr/Graphics/Color.h>

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

#include <imgui.h>

class EntityTreeView {
public:
    virtual ~EntityTreeView() = default;

    void update(const entt::registry& registry, float dt);

    void setSelectedEntity(const entt::handle e) { selectedEntity = e; };
    entt::const_handle getSelectedEntity() const { return selectedEntity; }
    bool hasSelectedEntity() const { return selectedEntity.entity() != entt::null; }

    virtual void displayExtraFilters(){};
    virtual bool displayEntityInView(entt::const_handle e, const std::string& label) const
    {
        return true;
    }

    virtual const std::string& getEntityDisplayName(entt::const_handle e) const;
    virtual RGBColor getDisplayColor(entt::const_handle e) const { return RGBColor{255, 255, 255}; }

private:
    entt::const_handle selectedEntity{};
    void updateEntityTreeUI(entt::const_handle e);

    ImGuiTextFilter filter;
};
