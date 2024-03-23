#pragma once

#include <string>

#include <imgui.h>

#include <glm/fwd.hpp>

#include <edbr/Math/Rect.h>

namespace devtools
{
void BeginPropertyTable();

namespace detail
{
    template<typename T>
    void DisplayPropertyImpl(const T& v);
    void DisplayPropertyImpl(const float& v);
    void DisplayPropertyImpl(const std::string& v);

    void DisplayPropertyImpl(const glm::vec2& v);
    void DisplayPropertyImpl(const glm::ivec2& v);
    void DisplayPropertyImpl(const glm::vec3& v);
    void DisplayPropertyImpl(const glm::vec4& v);
    void DisplayPropertyImpl(const glm::quat& v);

    void DisplayPropertyImpl(const math::IntRect& v);
    void DisplayPropertyImpl(const math::FloatRect& v);
}

template<typename T>
void DisplayProperty(const char* name, const T& v)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(name);
    ImGui::TableSetColumnIndex(1);
    detail::DisplayPropertyImpl(v);
};

void DisplayPropertyF(const char* name, const char* fmt, ...);
void DisplayColorProperty(const char* name, const glm::vec4& v);

void EndPropertyTable();

namespace detail
{
    template<typename T>
    void DisplayPropertyImpl(const T& v)

    {
        ImGui::TextUnformatted(std::to_string(v).c_str());
    }
} // end of namespace detail

} // end of namespace devtools
