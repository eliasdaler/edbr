#pragma once

#include <filesystem>

struct Scene;
class MeshCache;
class MaterialCache;
class GfxDevice;

namespace util
{
Scene loadGltfFile(
    GfxDevice& gfxDevice,
    MeshCache& meshCache,
    MaterialCache& materialCache,
    const std::filesystem::path& path);
}
