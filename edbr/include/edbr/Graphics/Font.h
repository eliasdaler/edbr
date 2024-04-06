#pragma once

#include <filesystem>
#include <functional>
#include <unordered_map>
#include <unordered_set>

#include <glm/vec2.hpp>

#include <edbr/Graphics/IdTypes.h>
#include <edbr/Graphics/Vulkan/GPUImage.h>
#include <edbr/Math/Rect.h>

class GfxDevice;

struct Glyph {
    glm::vec2 uv0; // top left
    glm::vec2 uv1; // bottom right
    glm::ivec2 bearing; // top left corner, relative to origin on the baseline
    int advance{0}; // offset to the next char in pixels
};

struct Font {
    bool load(GfxDevice& gfxDevice, const std::filesystem::path& path, int size);
    bool load(
        GfxDevice& gfxDevice,
        const std::filesystem::path& path,
        int size,
        const std::unordered_set<std::uint32_t>& neededCodePoints);

    glm::vec2 getGlyphAtlasSize() const;
    glm::vec2 getGlyphSize(std::uint32_t codePoint) const;

    void forEachGlyph(
        const std::string& text,
        std::function<void(const glm::vec2& pos, const glm::vec2& uv0, const glm::vec2& uv1)> f)
        const;

    // calculate bounding box - the return rect's position is top left corner
    math::FloatRect calculateTextBoundingBox(const std::string& text) const;

    // data
    std::unordered_map<std::uint32_t, Glyph> glyphs;
    std::unordered_set<std::uint32_t> loadedCodePoints;

    ImageId glyphAtlasID{NULL_IMAGE_ID};
    glm::vec2 atlasSize;

    int size{0};
    float lineSpacing{0}; // line spacing in pixels
    float ascenderPx{0.f};
    float descenderPx{0.f};
    bool loaded{false};
};
