#include <edbr/Graphics/SpriteRenderer.h>

#include <edbr/Graphics/Camera.h>
#include <edbr/Graphics/Font.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/Sprite.h>
#include <edbr/Math/Transform.h>

namespace
{
glm::vec2 getSpriteSize(const Sprite& sprite)
{
    return glm::abs(sprite.uv1 - sprite.uv0) * sprite.textureSize;
}
}

SpriteRenderer::SpriteRenderer(GfxDevice& gfxDevice) : gfxDevice(gfxDevice)
{}

void SpriteRenderer::init(VkFormat drawImageFormat)
{
    spriteDrawCommands.reserve(MAX_SPRITES);
    uiDrawingPipeline.init(gfxDevice, drawImageFormat, MAX_SPRITES);
    initialized = true;
}

void SpriteRenderer::cleanup()
{
    uiDrawingPipeline.cleanup(gfxDevice);
}

void SpriteRenderer::beginDrawing()
{
    assert(initialized && "SpriteRenderer::init not called");
    spriteDrawCommands.clear();
}

void SpriteRenderer::endDrawing()
{
    // do nothing
}

void SpriteRenderer::draw(VkCommandBuffer cmd, const GPUImage& drawImage)
{
    Camera uiCamera;
    uiCamera.initOrtho2D(static_cast<glm::vec2>(drawImage.getSize2D()));
    draw(cmd, drawImage, uiCamera.getViewProj());
}

void SpriteRenderer::draw(VkCommandBuffer cmd, const GPUImage& drawImage, const glm::mat4& viewProj)
{
    assert(initialized && "SpriteRenderer::init not called");
    // TODO: check that begin/endFrame are called

    TracyVkZoneC(gfxDevice.getTracyVkCtx(), cmd, "Sprite renderer", tracy::Color::Purple);

    const auto drawImageExtent = drawImage.getExtent2D();
    const auto drawSize = glm::vec2{drawImageExtent.width, drawImageExtent.height};

    uiDrawingPipeline.draw(cmd, gfxDevice, drawImage, viewProj, spriteDrawCommands);
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
    const LinearColor& color,
    int maxNumGlyphsToDraw)
{
    assert(font.glyphAtlasID != NULL_IMAGE_ID && "font wasn't loaded");
    const auto& atlasTexture = gfxDevice.getImage(font.glyphAtlasID);
    Sprite glyphSprite(atlasTexture);
    glyphSprite.color = color;

    int numGlyphsDrawn = 0;
    font.forEachGlyph(
        text,
        [this, &glyphSprite, &pos, &numGlyphsDrawn, &maxNumGlyphsToDraw](
            const glm::vec2& glyphPos, const glm::vec2& uv0, const glm::vec2& uv1) {
            if (numGlyphsDrawn >= maxNumGlyphsToDraw) {
                return;
            }
            glyphSprite.uv0 = uv0;
            glyphSprite.uv1 = uv1;
            drawSprite(glyphSprite, pos + glyphPos, 0.f, glm::vec2{1.f}, textShaderId);
            ++numGlyphsDrawn;
        });
}

void SpriteRenderer::drawRect(
    const math::FloatRect& rect,
    const LinearColor& color,
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
    rectSprite.texture = gfxDevice.getWhiteTextureID();
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

void SpriteRenderer::drawInsetRect(
    const math::FloatRect& rect,
    const LinearColor& color,
    float borderWidth,
    float rotation,
    const glm::vec2& scale,
    const glm::vec2& pivot)
{
    drawRect(rect, color, borderWidth, rotation, scale, pivot, true);
}

void SpriteRenderer::drawFilledRect(
    const math::FloatRect& rect,
    const LinearColor& color,
    float rotation,
    const glm::vec2& scale,
    const glm::vec2& pivot)
{
    Sprite rectSprite{};
    rectSprite.texture = gfxDevice.getWhiteTextureID();
    rectSprite.textureSize = glm::ivec2{1, 1};
    rectSprite.color = color;
    rectSprite.pivot = pivot;

    drawSprite(rectSprite, rect.getTopLeftCorner(), rotation, rect.getSize() * scale);
}
