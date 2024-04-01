#include <edbr/Util/ImGuiUtil.h>

#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/Vulkan/GPUImage.h>

#include <array>

namespace util
{
ImVec4 toImVec4(const glm::vec4& v)
{
    return {v.x, v.y, v.z, v.w};
}

ImVec4 toImVec4(const RGBColor& c)
{
    return {
        (float)c.r / 255.f,
        (float)c.g / 255.f,
        (float)c.b / 255.f,
        (float)c.a / 255.f,
    };
}

void ImGuiTextColored(const RGBColor& c, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ImGui::TextColoredV(toImVec4(c), fmt, args);
    va_end(args);
}

bool ImGuiColorEdit3(const char* name, LinearColor& c)
{
    const auto imCol = toImVec4(edbr::linearToRGB(c));
    std::array<float, 4> arrC{imCol.x, imCol.y, imCol.z, imCol.w};
    if (ImGui::ColorEdit3(name, arrC.data())) {
        const auto newCol = RGBColor{
            (std::uint8_t)(arrC[0] * 255.f),
            (std::uint8_t)(arrC[1] * 255.f),
            (std::uint8_t)(arrC[2] * 255.f),
            (std::uint8_t)(arrC[3] * 255.f)};
        c = edbr::rgbToLinear(newCol);
        return true;
    }
    return false;
}

void ImGuiImage(const GfxDevice& gfxDevice, const ImageId imageId, float scale)
{
    return ImGuiImage(gfxDevice.getImage(imageId), scale);
}

void ImGuiImage(const GPUImage& image, float scale)
{
    return ImGuiImage(
        (ImageId)image.getBindlessId(),
        glm::vec2{image.getExtent2D().width, image.getExtent2D().height},
        scale);
}

void ImGuiImage(const ImageId imageId, const glm::vec2& size, float scale)
{
    ImGui::Image(
        reinterpret_cast<ImTextureID>((std::uint64_t)imageId),
        ImVec2{size.x * scale, size.y * scale});
}

}
