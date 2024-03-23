#include "CustomEntityTreeView.h"

#include <imgui.h>

#include "Components.h"
#include "EntityUtil.h"

#include <edbr/Graphics/ColorUtil.h>

namespace
{
bool isLight(entt::const_handle e)
{
    return e.all_of<LightComponent>();
}

bool isStaticGeometry(entt::const_handle e)
{
    if (isLight(e)) {
        return false;
    }
    return !e.any_of<MovementComponent, TriggerComponent, PlayerSpawnComponent, CameraComponent>();
}

} // end of anonymous namespace

void CustomEntityTreeView::displayExtraFilters()
{
    ImGui::Checkbox("SG", &displayStaticGeometry);
    ImGui::SetItemTooltip("Display static geometry");
    ImGui::SameLine();
    ImGui::Checkbox("TSG", &displayTaggedStaticGeometry);
    ImGui::SetItemTooltip("Display tagged static geometry");
    ImGui::SameLine();
    ImGui::Checkbox("L", &displayLights);
    ImGui::SetItemTooltip("Display lights");
    ImGui::SameLine();
    ImGui::Checkbox("Tr", &displayTriggers);
    ImGui::SetItemTooltip("Display triggers");
    ImGui::SameLine();
    ImGui::Checkbox("C", &displayColliders);
    ImGui::SetItemTooltip("Display colliders");
}

const std::string& CustomEntityTreeView::getEntityDisplayName(entt::const_handle e) const
{
    if (const auto& tag = entityutil::getTag(e); !tag.empty()) {
        return tag;
    }
    return EntityTreeView::getEntityDisplayName(e);
}

bool CustomEntityTreeView::displayEntityInView(entt::const_handle e, const std::string& label) const
{
    if (!displayLights && isLight(e)) {
        return false;
    }
    if (!displayTriggers && e.all_of<TriggerComponent>()) {
        return false;
    }
    if (!displayColliders && label.starts_with("Collision")) {
        // FIXME: better way to detect colliders?
        return false;
    }
    if (isStaticGeometry(e)) {
        if (displayTaggedStaticGeometry && !entityutil::getTag(e).empty()) {
            return true;
        }
        return displayStaticGeometry;
    }
    return true;
}

glm::vec4 CustomEntityTreeView::getDisplayColor(entt::const_handle e) const
{
    if (isStaticGeometry(e)) {
        return util::colorRGB(200, 200, 200);
    } else if (isLight(e)) {
        return util::colorRGB(255, 255, 0);
    } else if (e.all_of<TriggerComponent>()) {
        return util::colorRGB(182, 228, 209);
    } else if (e.all_of<PlayerSpawnComponent>()) {
        return util::colorRGB(255, 180, 122);
    } else if (e.all_of<CameraComponent>()) {
        return util::colorRGB(237, 252, 223);
    }
    return glm::vec4{1.f};
}
