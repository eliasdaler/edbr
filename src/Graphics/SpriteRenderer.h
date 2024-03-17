#pragma once

#include <string>

#include <Graphics/Pipelines/SpriteDrawingPipeline.h>
#include <Graphics/SpriteDrawingCommand.h>
#include <Math/Rect.h>

class BaseRenderer;
struct Font;
class GfxDevice;
class Sprite;

class SpriteRenderer {
public:
    // keep in sync with sprite.frag
    static const std::uint32_t spriteShaderId = 0;
    static const std::uint32_t textShaderId = 1;

public:
    SpriteRenderer(BaseRenderer& renderer);
    void init(GfxDevice& gfxDevice, VkFormat drawImageFormat);
    void cleanup(GfxDevice& gfxDevice);

    void beginDrawing();
    void draw(VkCommandBuffer cmd, std::size_t frameIndex, const AllocatedImage& drawImage);

    void drawSprite(
        const Sprite& sprite,
        const glm::vec2& position,
        float rotation = 0.f, // rotation around Z in radians
        const glm::vec2& scale = glm::vec2{1.f},
        std::uint32_t shaderId = spriteShaderId);

    // Note: the sprite size should not be included in transform
    // it's calculated automatically - so if you want to draw the sprite at 2x
    // size, the transform should just have {2, 2, 1} scale
    void drawSprite(
        const Sprite& sprite,
        const glm::mat4& transform,
        std::uint32_t shaderId = spriteShaderId);

    void drawText(
        const Font& font,
        const std::string& text,
        const glm::vec2& pos,
        const glm::vec4& color = glm::vec4{1.f});

    // Draw unfilled rectangle
    // if insetBorder == true, the border is drawn inside the rect
    // otherwise it doesn't overlap the rect and is draw outside of it
    void drawRect(
        const math::FloatRect& rect,
        const glm::vec4& color,
        float borderWidth = 1.f,
        float rotation = 0.f, // rotation around Z in radians
        const glm::vec2& scale = glm::vec2{1.f},
        const glm::vec2& pivot = glm::vec2{0.f},
        bool insetBorder = false);

    void drawFilledRect(
        const math::FloatRect& rect,
        const glm::vec4& color,
        float rotation = 0.f, // rotation around Z in radians
        const glm::vec2& scale = glm::vec2{1.f},
        const glm::vec2& pivot = glm::vec2{0.f});

private:
    BaseRenderer& renderer;

    SpriteDrawingPipeline uiDrawingPipeline;

    static constexpr std::size_t MAX_SPRITES = 25000;
    std::vector<SpriteDrawCommand> spriteDrawCommands;
};
