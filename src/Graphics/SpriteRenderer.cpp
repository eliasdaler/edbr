#include "SpriteRenderer.h"

#include <Graphics/BaseRenderer.h>
#include <Graphics/Camera.h>
#include <Graphics/Font.h>
#include <Graphics/Sprite.h>
#include <Math/Transform.h>

#include <utf8.h>

namespace
{
glm::vec2 getSpriteSize(const Sprite& sprite)
{
    return glm::abs(sprite.uv1 - sprite.uv0) * sprite.textureSize;
}
}

SpriteRenderer::SpriteRenderer(BaseRenderer& renderer) : renderer(renderer)
{}

void SpriteRenderer::init(GfxDevice& gfxDevice, VkFormat drawImageFormat)
{
    spriteDrawCommands.reserve(MAX_SPRITES);
    uiDrawingPipeline
        .init(gfxDevice, drawImageFormat, renderer.getBindlessDescSetLayout(), MAX_SPRITES);
}

void SpriteRenderer::cleanup(GfxDevice& gfxDevice)
{
    uiDrawingPipeline.cleanup(gfxDevice);
}

void SpriteRenderer::beginDrawing()
{
    spriteDrawCommands.clear();
}

void SpriteRenderer::draw(
    VkCommandBuffer cmd,
    std::size_t frameIndex,
    const AllocatedImage& drawImage)
{
    const auto drawImageExtent = drawImage.getExtent2D();
    const auto drawSize = glm::vec2{drawImageExtent.width, drawImageExtent.height};

    Camera uiCamera;
    uiCamera.initOrtho2D(drawSize);
    const auto vp = uiCamera.getViewProj();

    uiDrawingPipeline
        .draw(cmd, renderer.getBindlessDescSet(), frameIndex, drawImage, vp, spriteDrawCommands);
}

void SpriteRenderer::drawSprite(
    const Sprite& sprite,
    const glm::vec2& position,
    float rotation,
    const glm::vec2& scale,
    std::uint32_t shaderId)
{
    Transform transform;
    transform.setPosition(glm::vec3{position, 0.f});
    if (rotation != 0.f) {
        transform.setHeading(glm::angleAxis(rotation, glm::vec3{0.f, 0.f, 1.f}));
    }
    transform.setScale(glm::vec3{scale, 1.f});

    drawSprite(sprite, transform.asMatrix(), shaderId);
}

void SpriteRenderer::drawSprite(
    const Sprite& sprite,
    const glm::mat4& transform,
    std::uint32_t shaderId)
{
    assert(sprite.texture != NULL_IMAGE_ID);

    auto tm = transform;
    const auto size = getSpriteSize(sprite);
    tm = glm::scale(tm, glm::vec3{size, 1.f});
    tm = glm::translate(tm, glm::vec3{-sprite.pivot, 0.f});

    spriteDrawCommands.push_back(SpriteDrawCommand{
        .transform = tm,
        .uv0 = sprite.uv0,
        .uv1 = sprite.uv1,
        .color = sprite.color,
        .textureId = sprite.texture,
        .shaderId = shaderId,
    });
}

void SpriteRenderer::drawText(
    const Font& font,
    const std::string& text,
    const glm::vec2& pos,
    const glm::vec4& color)
{
    float x = 0.0f;
    int lineNum = 0;

    auto it = text.begin();
    const auto e = text.end();
    std::uint32_t cp = 0;
    const auto& atlasTexture = renderer.getImage(font.glyphAtlasID);
    while (it != e) {
        cp = utf8::next(it, e);

        if (cp == static_cast<std::uint32_t>('\n')) {
            ++lineNum;
            x = 0.f;
            continue;
        }

        const auto& glyph = font.glyphs.contains(cp) ? font.glyphs.at(cp) : font.glyphs.at('?');
        const auto& uv0 = glyph.uv0;
        const auto& uv1 = glyph.uv1;

        float xpos = x + glyph.bearing.x;
        float ypos = -glyph.bearing.y + static_cast<float>(lineNum) * font.lineSpacing;

        Sprite glyphSprite(atlasTexture);
        glyphSprite.uv0 = uv0;
        glyphSprite.uv1 = uv1;
        glyphSprite.color = color;
        drawSprite(glyphSprite, pos + glm::vec2{xpos, ypos}, 0.f, glm::vec2{1.f}, textShaderId);

        x += glyph.advance;
    }
}

void SpriteRenderer::drawRect(
    const math::FloatRect& rect,
    const glm::vec4& color,
    float borderWidth,
    float rotation,
    const glm::vec2& scale,
    const glm::vec2& pivot,
    bool insetBorder)
{
    // border rects in rectangle local coordinate space
    std::array<math::FloatRect, 4> borderRects;
    if (insetBorder) {
        borderRects = {{
            {
                // top
                glm::vec2{0.f} - rect.getSize() * pivot,
                glm::vec2{rect.getSize().x, borderWidth},
            },
            {
                // left
                glm::vec2{0.f, borderWidth} - rect.getSize() * pivot,
                glm::vec2{borderWidth, rect.getSize().y - 2 * borderWidth},
            },
            {
                // right
                glm::vec2{rect.getSize().x - borderWidth, borderWidth} - rect.getSize() * pivot,
                glm::vec2{borderWidth, rect.getSize().y - 2 * borderWidth},
            },
            {
                // down
                glm::vec2{0.f, rect.getSize().y - borderWidth} - rect.getSize() * pivot,
                glm::vec2{rect.getSize().x, borderWidth},
            },
        }};
    } else {
        borderRects = {{
            {
                // top
                glm::vec2{-borderWidth, -borderWidth} - rect.getSize() * pivot,
                glm::vec2{rect.getSize().x + borderWidth * 2.f, borderWidth},
            },
            {
                // left
                glm::vec2{-borderWidth, 0.f} - rect.getSize() * pivot,
                glm::vec2{borderWidth, rect.getSize().y},
            },
            {
                // right
                glm::vec2{rect.getSize().x, 0.f} - rect.getSize() * pivot,
                glm::vec2{borderWidth, rect.getSize().y},
            },
            {
                // down
                glm::vec2{-borderWidth, rect.getSize().y} - rect.getSize() * pivot,
                glm::vec2{rect.getSize().x + borderWidth * 2.f, borderWidth},
            },
        }};
    }

    Sprite rectSprite{};
    rectSprite.texture = renderer.getWhiteTextureID();
    rectSprite.textureSize = glm::ivec2{1, 1};
    rectSprite.color = color;

    Transform parentTransform;
    parentTransform.setPosition(glm::vec3{rect.getTopLeftCorner(), 0.f});
    if (rotation != 0.f) {
        parentTransform.setHeading(glm::angleAxis(rotation, glm::vec3{0.f, 0.f, 1.f}));
    }
    parentTransform.setScale(glm::vec3{scale, 1.f});

    for (const auto& borderRect : borderRects) {
        Transform transform;
        transform.setPosition(glm::vec3{borderRect.getTopLeftCorner(), 0.f});
        transform.setScale(glm::vec3{borderRect.getSize(), 1.f});

        const auto tm = parentTransform.asMatrix() * transform.asMatrix();
        drawSprite(rectSprite, tm);
        /* More efficient code, but a bit harder to understand
           rectSprite.pivot = -borderRect.getPosition() / borderRect.getSize();
           drawSprite(rectSprite, rect.getTopLeftCorner(), rotation, borderRect.getSize() * scale);
        */
    }
}

void SpriteRenderer::drawFilledRect(
    const math::FloatRect& rect,
    const glm::vec4& color,
    float rotation,
    const glm::vec2& scale,
    const glm::vec2& pivot)
{
    Sprite rectSprite{};
    rectSprite.texture = renderer.getWhiteTextureID();
    rectSprite.textureSize = glm::ivec2{1, 1};
    rectSprite.color = color;
    rectSprite.pivot = pivot;

    drawSprite(rectSprite, rect.getTopLeftCorner(), rotation, rect.getSize() * scale);
}
