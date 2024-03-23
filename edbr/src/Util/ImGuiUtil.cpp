#include <edbr/Util/ImGuiUtil.h>

#include <array>

namespace util
{
ImVec4 toImVec4(const glm::vec4& v)
{
    return {v.x, v.y, v.z, v.w};
}

void ImGuiTextColored(const glm::vec4& c, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ImGui::TextColoredV(ImVec4(c.x, c.y, c.z, c.w), fmt, args);
    va_end(args);
}

bool ImGuiColorEdit3(const char* name, glm::vec4& c)
{
    std::array<float, 4> arrC{c.x, c.y, c.z, c.w};
    if (ImGui::ColorEdit3(name, arrC.data())) {
        c = {arrC[0], arrC[1], arrC[2], arrC[2]};
        return true;
    }
    return false;
}

}
