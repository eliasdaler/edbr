#include <edbr/DevTools/EntityInfoDisplayer.h>

#include <imgui.h>

#include <edbr/Graphics/ColorUtil.h>
#include <edbr/Util/ImGuiUtil.h>

void EntityInfoDisplayer::displayEntityInfo(entt::const_handle e, float dt)
{
    ImGui::Text("Entity id: %d", (int)e.entity());
    static const auto componentNameColor = util::colorRGB(220, 215, 252);
    for (const auto& displayer : componentDisplayers) {
        if (!displayer.componentExists(e)) {
            continue;
        }
        util::ImGuiTextColored(componentNameColor, "%s", displayer.componentName.c_str());
        if (displayer.displayFunc) {
            displayer.displayFunc(e);
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
