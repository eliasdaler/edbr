#include "Font.h"

#include <Graphics/GfxDevice.h>
#include <Graphics/Vulkan/Util.h>

#include <iostream>
#include <unordered_map>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H

static constexpr int GLYPH_ATLAS_SIZE = 1024;

bool Font::load(const GfxDevice& gfxDevice, const std::filesystem::path& path, int size)
{
    // only load ASCII by default
    std::unordered_set<std::uint32_t> neededCodePoints;
    for (std::uint32_t i = 0; i < 255; ++i) {
        neededCodePoints.insert(i);
    }
    return load(gfxDevice, path, size, neededCodePoints);
}

bool Font::load(
    const GfxDevice& gfxDevice,
    const std::filesystem::path& path,
    int size,
    const std::unordered_set<std::uint32_t>& neededCodePoints)
{
    std::cout << "Loading font: " << path << ", size=" << size
              << ", num glyphs to load= " << neededCodePoints.size() << std::endl;

    this->size = size;

    FT_Library ft;
    if (const auto err = FT_Init_FreeType(&ft)) {
        std::cout << "Failed to init FreeType Library: " << FT_Error_String(err) << std::endl;
        return false;
    }

    FT_Face face;
    if (const auto err = FT_New_Face(ft, path.string().c_str(), 0, &face)) {
        std::cout << "Failed to load font: " << FT_Error_String(err) << std::endl;
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, size);

    // because metrics.height is stored as 1/64th of pixels
    lineSpacing = face->size->metrics.height / 64.f;

    // TODO: allow to specify atlas size?
    int aw = GLYPH_ATLAS_SIZE;
    int ah = GLYPH_ATLAS_SIZE;
    std::vector<unsigned char> atlasData(aw * ah);

    int pen_x = 0;
    int pen_y = 0;

    int maxHeightInRow = 0;
    for (const auto& codepoint : neededCodePoints) {
        if (codepoint < 255 && !std::isprint((int)codepoint)) {
            continue;
        }

        if (const auto err = FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) {
            std::cout << "Failed to load glyph: " << FT_Error_String(err) << std::endl;
            continue;
        }

        const auto& bmp = face->glyph->bitmap;
        if (pen_x + (int)bmp.width >= aw) {
            pen_x = 0;
            pen_y += maxHeightInRow;
            maxHeightInRow = 0;
        }

        for (unsigned int row = 0; row < bmp.rows; ++row) {
            for (unsigned int col = 0; col < bmp.width; ++col) {
                int x = pen_x + col;
                int y = pen_y + row;
                atlasData[y * aw + x] = bmp.buffer[row * bmp.pitch + col];
            }
        }

        auto g = Glyph{
            .uv0 = {pen_x / (float)aw, pen_y / (float)ah},
            .uv1 = {(pen_x + bmp.width) / (float)aw, (pen_y + bmp.rows) / (float)ah},
            .bearing = {face->glyph->bitmap_left, face->glyph->bitmap_top},
            .advance = (int)(face->glyph->advance.x >> 6),
        };
        glyphs.emplace(codepoint, std::move(g));

        pen_x += bmp.width;
        if ((int)bmp.rows > maxHeightInRow) {
            maxHeightInRow = (int)bmp.rows;
        }
    }

    { // upload data to GPU
        glyphAtlas = gfxDevice.createImage({
            .format = VK_FORMAT_R8_UNORM,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .extent = {(std::uint32_t)aw, (std::uint32_t)ah, 1},
        });
        gfxDevice.uploadImageData(glyphAtlas, atlasData.data());

        const auto label = "glyph_atlas: " + path.string();
        vkutil::addDebugLabel(gfxDevice.getDevice(), glyphAtlas.image, label.c_str());
    }

    loadedCodePoints = neededCodePoints;

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return true;
}

void Font::destroy(const GfxDevice& gfxDevice)
{
    gfxDevice.destroyImage(glyphAtlas);
}

glm::vec2 Font::getGlyphAtlasSize() const
{
    const auto atlasSize = glyphAtlas.getExtent2D();
    return glm::vec2{atlasSize.width, atlasSize.height};
}

glm::vec2 Font::getGlyphSize(std::uint32_t codePoint) const
{
    auto it = glyphs.find(codePoint);
    if (it == glyphs.end()) {
        return {};
    }
    const auto& g = it->second;
    const auto atlasSize = glyphAtlas.getExtent2D();
    return (g.uv1 - g.uv0) * getGlyphAtlasSize();
}
