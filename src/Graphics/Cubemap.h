#pragma once

#include <filesystem>

struct AllocatedImage;
class Renderer;

namespace graphics
{
AllocatedImage loadCubemap(const Renderer& renderer, const std::filesystem::path& imagesDir);
}
