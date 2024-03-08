#pragma once

#include <filesystem>

struct AllocatedImage;
class GfxDevice;

namespace graphics
{
AllocatedImage loadCubemap(const GfxDevice& gfxDevice, const std::filesystem::path& imagesDir);
}
