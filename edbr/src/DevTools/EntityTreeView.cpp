#include <edbr/DevTools/EntityTreeView.h>

#include <edbr/Util/ImGuiUtil.h>

#include <fmt/printf.h>
#include <imgui.h>

#include <edbr/ECS/Components/HierarchyComponent.h>
#include <edbr/ECS/Components/MetaInfoComponent.h>

void EntityTreeView::update(entt::registry& registry, float dt)
{
    displayExtraFilters();
    filter.Draw("search##name_filter");

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::BeginChild("##entities");
    for (const auto&& [e, hc] : registry.view<HierarchyComponent>().each()) {
        if (!hc.hasParent()) { // start from root nodes
            updateEntityTreeUI(entt::handle{registry, e});
        }
    }
    ImGui::EndChild();
}

const std::string& EntityTreeView::getEntityDisplayName(entt::const_handle e) const
{
    const auto& mic = e.get<MetaInfoComponent>();
    return !mic.sceneNodeName.empty() ? mic.sceneNodeName : mic.prefabName;
}

void EntityTreeView::updateEntityTreeUI(entt::handle e)
{
    const auto label =
        fmt::format("{} (id={})", getEntityDisplayName(e), (std::uint32_t)e.entity());
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
            updateEntityTreeUI(child);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}
