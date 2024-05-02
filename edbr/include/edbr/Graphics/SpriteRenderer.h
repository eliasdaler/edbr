#pragma once

#include <string>

#include <edbr/Graphics/Pipelines/SpriteDrawingPipeline.h>
#include <edbr/Graphics/SpriteDrawingCommand.h>
#include <edbr/Math/Rect.h>

struct Font;
class GfxDevice;
class Sprite;
struct GPUImage;

class SpriteRenderer {
public:
    // keep in sync with sprite.frag
    static const std::uint32_t spriteShaderId = 0;
    static const std::uint32_t textShaderId = 1;

public:
    SpriteRenderer(GfxDevice& gfxDevice);
    void init(VkFormat drawImageFormat);
    void cleanup();

    void beginDrawing();
    void draw(VkCommandBuffer cmd, const GPUImage& drawImage);
    void endDrawing();

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
        const LinearColor& color = LinearColor{0.f, 0.f, 0.f, 1.f},
        int maxNumGlyphsToDraw = std::numeric_limits<int>::max());

    // Draw unfilled rectangle
    // if insetBorder == true, the border is drawn inside the rect
    // otherwise it doesn't overlap the rect and is draw outside of it
    void drawRect(
        const math::FloatRect& rect,
        const LinearColor& color,
        float borderWidth = 1.f,
        float rotation = 0.f, // rotation around Z in radians
        const glm::vec2& scale = glm::vec2{1.f},
        const glm::vec2& pivot = glm::vec2{0.f},
        bool insetBorder = false);

    void drawInsetRect(
        const math::FloatRect& rect,
        const LinearColor& color,
        float borderWidth = 1.f,
        float rotation = 0.f, // rotation around Z in radians
        const glm::vec2& scale = glm::vec2{1.f},
        const glm::vec2& pivot = glm::vec2{0.f});

    void drawFilledRect(
        const math::FloatRect& rect,
        const LinearColor& color,
        float rotation = 0.f, // rotation around Z in radians
        const glm::vec2& scale = glm::vec2{1.f},
        const glm::vec2& pivot = glm::vec2{0.f});

    GfxDevice& getGfxDevice() { return gfxDevice; }

private:
    GfxDevice& gfxDevice;
    bool initialized{false};

    SpriteDrawingPipeline uiDrawingPipeline;

    static constexpr std::size_t MAX_SPRITES = 25000;
    std::vector<SpriteDrawCommand> spriteDrawCommands;
};
