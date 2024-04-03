#pragma once

#include <filesystem>

struct Scene;
class BaseRenderer;
class MaterialCache;
class GfxDevice;

namespace util
{
Scene loadGltfFile(
    BaseRenderer& renderer,
    GfxDevice& gfxDevice,
    MaterialCache& materialCache,
    const std::filesystem::path& path);
}
