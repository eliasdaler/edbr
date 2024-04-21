#include <edbr/DevTools/EntityInfoDisplayer.h>

#include <imgui.h>

#include <edbr/Util/ImGuiUtil.h>

void EntityInfoDisplayer::displayEntityInfo(entt::const_handle e, float dt)
{
    ImGui::Text("Entity id: %d", (int)e.entity());
    static const auto componentNameColor = RGBColor{220, 215, 252};
    for (const auto& displayer : componentDisplayers) {
        if (!displayer.componentExists(e)) {
            continue;
        }
        switch (displayer.style) {
        case DisplayStyle::Default:
            util::ImGuiTextColored(componentNameColor, "%s", displayer.componentName.c_str());
            if (displayer.displayFunc) {
                displayer.displayFunc(e);
            }
            break;
        case DisplayStyle::CollapsedByDefault:
            util::ImGuiPushTextStyleColor(componentNameColor);
            if (ImGui::TreeNode(displayer.componentName.c_str())) {
                ImGui::PopStyleColor();
                if (displayer.displayFunc) {
                    displayer.displayFunc(e);
                }
                ImGui::TreePop();
            } else {
                ImGui::PopStyleColor();
            }
            break;
        default:
            assert(false);
            break;
        }
    }
}

bool EntityInfoDisplayer::displayerRegistered(const std::string& componentName) const
{
    for (const auto& displayer : componentDisplayers) {
        if (displayer.componentName == componentName) {
            return true;
        }
    }
    return false;
}
