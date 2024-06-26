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

bool Font::load(
    GfxDevice& gfxDevice,
    const std::filesystem::path& path,
    int size,
    bool antialiasing)
{
    // only load ASCII by default
    std::unordered_set<std::uint32_t> neededCodePoints;
    for (std::uint32_t i = 0; i < 255; ++i) {
        neededCodePoints.insert(i);
    }
    return load(gfxDevice, path, size, neededCodePoints, antialiasing);
}

bool Font::load(
    GfxDevice& gfxDevice,
    const std::filesystem::path& path,
    int size,
    const std::unordered_set<std::uint32_t>& neededCodePoints,
    bool antialiasing)
{
    std::cout << "Loading font: " << path << ", size=" << size
              << ", num glyphs to load=" << neededCodePoints.size() << std::endl;

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

    // because metrics.height etc. is stored as 1/64th of pixels
    lineSpacing = face->size->metrics.height / 64.f;
    ascenderPx = face->size->metrics.ascender / 64.f;
    descenderPx = face->size->metrics.descender / 64.f;

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

        auto loadFlags = FT_LOAD_RENDER;
        if (!antialiasing) {
            loadFlags |= FT_LOAD_MONOCHROME;
        }
        if (const auto err = FT_Load_Char(face, codepoint, loadFlags)) {
            std::cout << "Failed to load glyph: " << FT_Error_String(err) << std::endl;
            continue;
        }

        const auto& bmp = face->glyph->bitmap;
        if (pen_x + (int)bmp.width >= aw) {
            pen_x = 0;
            pen_y += maxHeightInRow;
            maxHeightInRow = 0;
        }

        if (antialiasing) {
            for (unsigned int row = 0; row < bmp.rows; ++row) {
                for (unsigned int col = 0; col < bmp.width; ++col) {
                    int x = pen_x + col;
                    int y = pen_y + row;
                    atlasData[y * aw + x] = bmp.buffer[row * bmp.pitch + col];
                }
            }
        } else {
            assert(bmp.pixel_mode == FT_PIXEL_MODE_MONO);
            const auto* pixels = bmp.buffer;
            auto* dest = &atlasData[pen_y * aw + pen_x];
            for (int y = 0; y < (int)bmp.rows; y++) {
                for (int x = 0; x < (int)bmp.width; x++) {
                    std::uint8_t v = ((pixels[x / 8]) & (1 << (7 - (x % 8)))) ? 255 : 0;
                    dest[y * aw + x] = v;
                }

                pixels += bmp.pitch;
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
        float ypos = (ascenderPx - glyph.bearing.y) + static_cast<float>(lineNum) * lineSpacing;

        f({xpos, ypos}, uv0, uv1);

        x += glyph.advance;
    }
}

math::FloatRect Font::calculateTextBoundingBox(const std::string& text) const
{
    if (text.empty()) {
        return {};
    }

    float x = 0.0f;
    int numLines = 1;
    std::uint32_t cp = 0;
    float maxAdvance = 0.f;

    auto it = text.begin();
    const auto e = text.end();
    while (it != e) {
        cp = utf8::next(it, e);

        if (cp == static_cast<std::uint32_t>('\n')) {
            ++numLines;
            x = 0.f;
            continue;
        }

        const auto& glyph = glyphs.contains(cp) ? glyphs.at(cp) : glyphs.at('?');
        x += glyph.advance;
        maxAdvance = std::max(maxAdvance, x);
    }

    return {0.f, 0.f, maxAdvance, (float)(numLines) * (ascenderPx - descenderPx)};
}
