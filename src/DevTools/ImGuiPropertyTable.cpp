#include "ImGuiPropertyTable.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <imgui.h>

#include <array>

namespace devtools
{
void BeginPropertyTable()
{
    ImGui::BeginTable("##table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
}

void EndPropertyTable()
{
    ImGui::EndTable();
}

namespace detail
{
    void DisplayPropertyImpl(const float& v)
    {
        ImGui::Text("%.2f", v);
    }

    void DisplayPropertyImpl(const std::string& v)
    {
        ImGui::TextUnformatted(v.c_str());
    }

    void DisplayPropertyImpl(const glm::vec2& v)
    {
        ImGui::Text("(%.2f, %.2f)", v.x, v.y);
    }

    void DisplayPropertyImpl(const glm::ivec2& v)
    {
        ImGui::Text("(%d, %d)", v.x, v.y);
    }

    void DisplayPropertyImpl(const glm::vec3& v)
    {
        ImGui::Text("(%.2f, %.2f, %.2f)", v.x, v.y, v.z);
    }

    void DisplayPropertyImpl(const glm::vec4& v)
    {
        ImGui::Text("(%.2f, %.2f, %.2f, %.2f)", v.x, v.y, v.z, v.w);
    }

    void DisplayPropertyImpl(const glm::quat& v)
    {
        ImGui::Text("(%.2f, %.2f, %.2f, %.2f)", v.w, v.x, v.y, v.z);
    }

    void DisplayPropertyImpl(const math::IntRect& v)
    {
        ImGui::Text("(%d, %d, %d, %d)", v.left, v.top, v.width, v.height);
    }

    void DisplayPropertyImpl(const math::FloatRect& v)
    {
        ImGui::Text("(%.2f, %.2f, %.2f, %.2f)", v.left, v.top, v.width, v.height);
    }
}

void DisplayPropertyF(const char* name, const char* fmt, ...)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(name);
    ImGui::TableSetColumnIndex(1);
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
}

void DisplayColorProperty(const char* name, const glm::vec4& v)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(name);
    ImGui::TableSetColumnIndex(1);
    const auto flags = ImGuiColorEditFlags_Float;
    std::array<float, 4> arr{v.x, v.y, v.z, v.w};
    ImGui::ColorEdit3("##Color", arr.data(), flags);
}

}
