#pragma once

#include <filesystem>

struct GPUImage;
class GfxDevice;

namespace graphics
{
GPUImage loadCubemap(GfxDevice& gfxDevice, const std::filesystem::path& imagesDir);
}
