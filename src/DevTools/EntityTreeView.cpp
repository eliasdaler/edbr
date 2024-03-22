#include "EntityTreeView.h"

#include "Components.h"

#include <Graphics/ColorUtil.h>
#include <Util/ImGuiUtil.h>

#include <fmt/printf.h>
#include <imgui.h>

#include <ECS/Components/MetaInfoComponent.h>

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

void EntityTreeView::update(const entt::registry& registry, float dt)
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
    filter.Draw("search##name_filter");

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::BeginChild("##entities");
    for (const auto&& [e, hc] : registry.view<HierarchyComponent>().each()) {
        if (!hc.hasParent()) { // start from root nodes
            updateEntityTreeUI(entt::const_handle{registry, e});
        }
    }
    ImGui::EndChild();
}

namespace
{
const std::string& getEntityDisplayName(entt::const_handle e)
{
    if (const auto& tag = e.get<TagComponent>().getTag(); !tag.empty()) {
        return tag;
    }
    const auto& mic = e.get<MetaInfoComponent>();
    return !mic.sceneNodeName.empty() ? mic.sceneNodeName : mic.prefabName;
}
}

void EntityTreeView::updateEntityTreeUI(entt::const_handle e)
{
    const auto label =
        fmt::format("{} (id={})", getEntityDisplayName(e), (std::uint32_t)e.entity());
    /* if (e.isPersistent()) {
        label += "(p)";
    } */
    if (!filter.PassFilter(label.c_str())) {
        return;
    }

    const auto& hc = e.get<HierarchyComponent>();
    if (!hc.hasParent() && // always display entities with parents
        !displayEntityInView(e, label)) {
        return;
    }

    ImGui::PushID((int)e.entity());
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                               ImGuiTreeNodeFlags_OpenOnDoubleClick |
                               ImGuiTreeNodeFlags_DefaultOpen;
    if (hc.children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (e == selectedEntity) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, util::toImVec4(getDisplayColor(e)));
    bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags);
    ImGui::PopStyleColor();

    if (isOpen) {
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            selectedEntity = e;
        }
        for (const auto& child : hc.children) {
            updateEntityTreeUI(entt::const_handle{*e.registry(), child});
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

bool EntityTreeView::displayEntityInView(entt::const_handle e, const std::string& label) const
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
        if (displayTaggedStaticGeometry && !e.get<TagComponent>().getTag().empty()) {
            return true;
        }
        return displayStaticGeometry;
    }
    return true;
}

glm::vec4 EntityTreeView::getDisplayColor(entt::const_handle e) const
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
