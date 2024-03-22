#pragma once

#include <filesystem>

struct Scene;
class BaseRenderer;

namespace util
{
Scene loadGltfFile(BaseRenderer& renderer, const std::filesystem::path& path);
}
