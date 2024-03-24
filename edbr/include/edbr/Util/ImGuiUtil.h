#pragma once

#include <imgui.h>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <edbr/Graphics/IdTypes.h>

struct GPUImage;
struct GfxDevice;

namespace util
{
ImVec4 toImVec4(const glm::vec4& v);
void ImGuiTextColored(const glm::vec4& c, const char* fmt, ...);
bool ImGuiColorEdit3(const char* name, glm::vec4& c);

void ImGuiImage(const GfxDevice& gfxDevice, const ImageId imageId, float scale = 1.f);
void ImGuiImage(const GPUImage& image, float scale = 1.f);
void ImGuiImage(const ImageId imageId, const glm::vec2& size, float scale = 1.f);
}
