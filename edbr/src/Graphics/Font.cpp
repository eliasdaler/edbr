#include <edbr/Graphics/Font.h>

#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/Vulkan/Util.h>

#include <iostream>
#include <unordered_map>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H

#include <utf8.h>

static constexpr int GLYPH_ATLAS_SIZE = 1024;

bool Font::load(GfxDevice& gfxDevice, const std::filesystem::path& path, int size)
{
    // only load ASCII by default
    std::unordered_set<std::uint32_t> neededCodePoints;
    for (std::uint32_t i = 0; i < 255; ++i) {
        neededCodePoints.insert(i);
    }
    return load(gfxDevice, path, size, neededCodePoints);
}

bool Font::load(
    GfxDevice& gfxDevice,
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
        const auto label = "glyph_atlas: " + path.string();
        glyphAtlasID = gfxDevice.createImage(
            {
                .format = VK_FORMAT_R8_UNORM,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .extent = {(std::uint32_t)aw, (std::uint32_t)ah, 1},
            },
            label.c_str(),
            atlasData.data());
        atlasSize = glm::vec2{aw, ah};
    }

    loadedCodePoints = neededCodePoints;

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    loaded = true;

    return true;
}

glm::vec2 Font::getGlyphAtlasSize() const
{
    return atlasSize;
}

glm::vec2 Font::getGlyphSize(std::uint32_t codePoint) const
{
    auto it = glyphs.find(codePoint);
    if (it == glyphs.end()) {
        return {};
    }
    const auto& g = it->second;
    return (g.uv1 - g.uv0) * atlasSize;
}

void Font::forEachGlyph(
    const std::string& text,
    std::function<void(const glm::vec2& pos, const glm::vec2& uv0, const glm::vec2& uv1)> f) const
{
    float x = 0.0f;
    int lineNum = 0;

    auto it = text.begin();
    const auto e = text.end();
    std::uint32_t cp = 0;
    while (it != e) {
        cp = utf8::next(it, e);

        if (cp == static_cast<std::uint32_t>('\n')) {
            ++lineNum;
            x = 0.f;
            continue;
        }

        const auto& glyph = glyphs.contains(cp) ? glyphs.at(cp) : glyphs.at('?');
        const auto& uv0 = glyph.uv0;
        const auto& uv1 = glyph.uv1;

        float xpos = x + glyph.bearing.x;
        float ypos = -glyph.bearing.y + static_cast<float>(lineNum) * lineSpacing;

        f({xpos, ypos}, uv0, uv1);

        x += glyph.advance;
    }
}

math::FloatRect Font::calculateTextBoundingBox(const std::string& text) const
{
    if (text.empty()) {
        return {};
    }

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    forEachGlyph(
        text,
        [this,
         &minX,
         &maxX,
         &minY,
         &maxY](const glm::vec2& glyphPos, const glm::vec2& uv0, const glm::vec2& uv1) {
            // no mirroring allowed
            assert(uv1.x >= uv0.x);
            assert(uv1.y >= uv0.y);

            float gx = glyphPos.x;
            float gy = glyphPos.y;
            float gw = (uv1.x - uv0.x) * atlasSize.x;
            float gh = (uv1.y - uv0.y) * atlasSize.y;

            minX = std::min(minX, gx);
            minY = std::min(minY, gy);

            maxX = std::max(maxX, gx + gw);
            maxY = std::max(maxY, gy + gh);
        });

    return math::FloatRect{minX, minY, maxX - minX, maxY - minY};
}
