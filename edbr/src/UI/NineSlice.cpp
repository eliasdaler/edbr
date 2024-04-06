#include <edbr/UI/NineSlice.h>

#include <edbr/Graphics/CoordUtil.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/Sprite.h>
#include <edbr/Graphics/SpriteRenderer.h>

namespace ui
{
void NineSlice::setStyle(ui::NineSliceStyle s)
{
    style = std::move(s);
    setSize(size);
}

void NineSlice::setSize(const glm::vec2& newSize)
{
    if (newSize == size) {
        return;
    }
    size = newSize;
}

namespace
{
    glm::vec2 getUV0(math::IntRect r, glm::ivec2 ts, bool flipX, bool flipY)
    {
        auto uv1 = edbr::util::pixelCoordToUV({r.left, r.top}, ts);
        auto uv2 = edbr::util::pixelCoordToUV({r.left + r.width, r.top + r.height}, ts);

        if (flipX) {
            uv1.x = uv2.x;
        }

        if (flipY) {
            uv1.y = uv2.y;
        }

        return uv1;
    }

    glm::vec2 getUV1(math::IntRect r, glm::ivec2 ts, bool flipX, bool flipY)
    {
        auto uv1 = edbr::util::pixelCoordToUV({r.left, r.top}, ts);
        auto uv2 = edbr::util::pixelCoordToUV({r.left + r.width, r.top + r.height}, ts);

        if (flipX) {
            uv2.x = uv1.x;
        }

        if (flipY) {
            uv2.y = uv1.y;
        }

        return uv2;
    }
}

void NineSlice::draw(SpriteRenderer& spriteRenderer) const
{
    // Origin is at upper-left corner of (4)
    // The size of (4) is this->size
    // --------------------------
    // |0|         1          |2|
    // --------------------------
    // | |                    | |
    // |3|         4          |5|
    // | |                    | |
    // --------------------------
    // |6|         7          |8|
    // --------------------------
    //
    // 0 - up left corner
    // 1 - up horizontal border
    // ...
    // 4 - contents
    // ... etc.

    const auto& texture = spriteRenderer.getGfxDevice().getImage(style.texture);
    auto cornerSize = static_cast<glm::vec2>(style.coords.corner.getSize());

    assert(style.texture && "style was not set?");
    const auto ts = texture.getSize2D();

    Sprite sprite(texture);

    // up left corner
    sprite.uv0 = getUV0(style.coords.corner, ts, false, false);
    sprite.uv1 = getUV1(style.coords.corner, ts, false, false);
    spriteRenderer.drawSprite(sprite, position - cornerSize);

    // up horizontal border
    sprite.uv0 = getUV0(style.coords.horizonal, ts, false, false);
    sprite.uv1 = getUV1(style.coords.horizonal, ts, false, false);
    spriteRenderer
        .drawSprite(sprite, position + glm::vec2{0.f, -cornerSize.y}, 0.f, glm::vec2{size.x, 1.f});

    // up right corner
    sprite.uv0 = getUV0(style.coords.corner, ts, true, false);
    sprite.uv1 = getUV1(style.coords.corner, ts, true, false);
    spriteRenderer.drawSprite(sprite, position + glm::vec2{size.x, -cornerSize.y});

    // left vertical border
    sprite.uv0 = getUV0(style.coords.vertical, ts, false, false);
    sprite.uv1 = getUV1(style.coords.vertical, ts, false, false);
    spriteRenderer
        .drawSprite(sprite, position + glm::vec2{-cornerSize.x, 0.f}, 0.f, glm::vec2{1.f, size.y});

    // contents
    sprite.uv0 = getUV0(style.coords.contents, ts, false, false);
    sprite.uv1 = getUV1(style.coords.contents, ts, false, false);
    spriteRenderer.drawSprite(sprite, position, 0.f, size);

    // right vertical border
    sprite.uv0 = getUV0(style.coords.vertical, ts, true, false);
    sprite.uv1 = getUV1(style.coords.vertical, ts, true, false);
    spriteRenderer
        .drawSprite(sprite, position + glm::vec2{size.x, 0.f}, 0.f, glm::vec2{1.f, size.y});

    // down left corner
    sprite.uv0 = getUV0(style.coords.corner, ts, false, true);
    sprite.uv1 = getUV1(style.coords.corner, ts, false, true);
    spriteRenderer.drawSprite(sprite, position + glm::vec2{-cornerSize.y, size.y});

    // down horizontal border
    sprite.uv0 = getUV0(style.coords.horizonal, ts, false, true);
    sprite.uv1 = getUV1(style.coords.horizonal, ts, false, true);
    spriteRenderer
        .drawSprite(sprite, position + glm::vec2{0.f, size.y}, 0.f, glm::vec2{size.x, 1.f});

    // down right corner
    sprite.uv0 = getUV0(style.coords.corner, ts, true, true);
    sprite.uv1 = getUV1(style.coords.corner, ts, true, true);
    spriteRenderer.drawSprite(sprite, position + size);
}

} // end of namespace UI
