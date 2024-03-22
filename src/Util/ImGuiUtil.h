#pragma once

#include <imgui.h>

#include <glm/vec4.hpp>

namespace util
{
ImVec4 toImVec4(const glm::vec4& v);
void ImGuiTextColored(const glm::vec4& c, const char* fmt, ...);
bool ImGuiColorEdit3(const char* name, glm::vec4& c);
}
