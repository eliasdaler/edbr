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

void NineSlice::draw(
    GfxDevice& gfxDevice,
    SpriteRenderer& spriteRenderer,
    const glm::vec2& position,
    const glm::vec2& size) const
{
    // Origin is at top-left corner
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
    // 0 - top left corner
    // 1 - top border
    // ...
    // 4 - contents
    // ... etc.

    const auto& texture = gfxDevice.getImage(style.texture);

    assert(style.texture && "style was not set?");
    const auto ts = texture.getSize2D();

    Sprite sprite(texture);

    // top left corner
    sprite.setTextureRect(style.textureCoords.topLeftCorner);
    spriteRenderer.drawSprite(gfxDevice, sprite, position);

    // top border
    sprite.setTextureRect(style.textureCoords.topBorder);
    float topBorderWidth =
        size.x - style.textureCoords.topLeftCorner.width - style.textureCoords.topRightCorner.width;
    spriteRenderer.drawSprite(
        gfxDevice,
        sprite,
        position + glm::vec2{(float)style.textureCoords.topLeftCorner.width, 0.f},
        0.f,
        {topBorderWidth / (float)style.textureCoords.topBorder.width, 1.f});

    // top right corner
    sprite.setTextureRect(style.textureCoords.topRightCorner);
    spriteRenderer.drawSprite(
        gfxDevice,
        sprite,
        position + glm::vec2{style.textureCoords.topLeftCorner.width + topBorderWidth, 0.f});

    // left border
    float leftBorderHeight = size.y - style.textureCoords.topLeftCorner.height -
                             style.textureCoords.bottomLeftCorner.height;
    sprite.setTextureRect(style.textureCoords.leftBorder);
    spriteRenderer.drawSprite(
        gfxDevice,
        sprite,
        position + glm::vec2{0.f, style.textureCoords.topLeftCorner.height},
        0.f,
        {1.f, leftBorderHeight / (float)style.textureCoords.leftBorder.height});

    // contents
    sprite.setTextureRect(style.textureCoords.contents);
    spriteRenderer.drawSprite(
        gfxDevice,
        sprite,
        position +
            glm::vec2{
                style.textureCoords.topLeftCorner.width, style.textureCoords.topLeftCorner.height},
        0.f,
        {topBorderWidth / (float)style.textureCoords.contents.width,
         leftBorderHeight / (float)style.textureCoords.contents.height});

    // right border
    sprite.setTextureRect(style.textureCoords.rightBorder);
    spriteRenderer.drawSprite(
        gfxDevice,
        sprite,
        position +
            glm::vec2{
                style.textureCoords.topLeftCorner.width + topBorderWidth,
                style.textureCoords.topLeftCorner.height},
        0.f,
        {1.f, leftBorderHeight / (float)style.textureCoords.rightBorder.height});

    // bottom left corner
    sprite.setTextureRect(style.textureCoords.bottomLeftCorner);
    spriteRenderer.drawSprite(
        gfxDevice,
        sprite,
        position + glm::vec2{0.f, leftBorderHeight + style.textureCoords.topLeftCorner.height});

    // bottom border
    sprite.setTextureRect(style.textureCoords.bottomBorder);
    spriteRenderer.drawSprite(
        gfxDevice,
        sprite,
        position +
            glm::vec2{
                style.textureCoords.topLeftCorner.width,
                leftBorderHeight + style.textureCoords.topLeftCorner.height},
        0.f,
        {topBorderWidth / (float)style.textureCoords.bottomBorder.width, 1.f});

    // bottom right corner
    sprite.setTextureRect(style.textureCoords.bottomRightBorder);
    spriteRenderer.drawSprite(
        gfxDevice,
        sprite,
        position + glm::vec2{
                       style.textureCoords.topLeftCorner.width + topBorderWidth,
                       leftBorderHeight + style.textureCoords.topLeftCorner.height});
}

} // end of namespace UI
