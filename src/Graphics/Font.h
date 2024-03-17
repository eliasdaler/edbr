#pragma once

#include <filesystem>
#include <unordered_map>
#include <unordered_set>

#include <glm/vec2.hpp>

#include <Graphics/IdTypes.h>
#include <Graphics/Vulkan/Types.h>

class GfxDevice;
class BaseRenderer;

struct Glyph {
    glm::vec2 uv0; // top left
    glm::vec2 uv1; // bottom right
    glm::ivec2 bearing; // top left corner, relative to origin on the baseline
    int advance{0}; // offset to the next char in pixels
};

struct Font {
    bool load(
        const GfxDevice& gfxDevice,
        BaseRenderer& renderer,
        const std::filesystem::path& path,
        int size);
    bool load(
        const GfxDevice& gfxDevice,
        BaseRenderer& renderer,
        const std::filesystem::path& path,
        int size,
        const std::unordered_set<std::uint32_t>& neededCodePoints);

    glm::vec2 getGlyphAtlasSize() const;
    glm::vec2 getGlyphSize(std::uint32_t codePoint) const;

    std::unordered_map<std::uint32_t, Glyph> glyphs;
    std::unordered_set<std::uint32_t> loadedCodePoints;

    ImageId glyphAtlasID{NULL_IMAGE_ID};
    glm::vec2 atlasSize;

    int size{0};
    float lineSpacing{0}; // line spacing in pixels
};
