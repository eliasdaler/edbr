#pragma once

#include <imgui.h>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <edbr/Graphics/Color.h>
#include <edbr/Graphics/IdTypes.h>

#include <edbr/Util/ImGuiGeneric.h>

struct GPUImage;
class GfxDevice;

namespace util
{
ImVec4 toImVec4(const glm::vec4& v);
ImVec4 toImVec4(const RGBColor& c);
void ImGuiPushTextStyleColor(const RGBColor& c);
void ImGuiTextColored(const RGBColor& c, const char* fmt, ...);
bool ImGuiColorEdit3(const char* name, LinearColor& c);

void ImGuiImage(const GfxDevice& gfxDevice, const ImageId imageId, float scale = 1.f);
void ImGuiImage(const GPUImage& image, float scale = 1.f);
void ImGuiImage(const ImageId imageId, const glm::vec2& size, float scale = 1.f);
}
