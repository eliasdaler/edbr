#pragma once

#include <memory>

#include <edbr/Graphics/Bouncer.h>
#include <edbr/Graphics/Font.h>
#include <edbr/UI/Element.h>

class GfxDevice;
class SpriteRenderer;

namespace ui
{
struct TextElement;
struct ImageElement;
}

class AudioManager;

class DialogueBox {
public:
    void init(GfxDevice& gfxDevice, AudioManager& audioManager);

    void update(const glm::vec2& screenSize, float dt);

    void draw(SpriteRenderer& spriteRenderer) const;

    void setVisible(bool b);
    bool isVisible() const;
    void setText(const std::string t);
    void setSpeakerName(const std::string t);

    bool isWholeTextDisplayed() const;

private:
    void createUI(GfxDevice& gfxDevice);
    ui::TextElement& getMainTextElement();
    ui::TextElement& getMenuNameTextElement();
    ui::ImageElement& getMoreTextImageElement();

    Font defaultFont;
    std::unique_ptr<ui::Element> dialogueBoxUI;

    std::string text;

    float textSoundTriggerSpeed{0.08f};
    float textSoundTriggerValue{0.f};

    float currentTextDelay{0.f};
    float delayValue{0.f};
    float punctuationDelayTime{0.1f};
    bool wasPunctuation{false};
    std::unordered_set<std::size_t> delaysDone;
    float numGlyphsToDraw{0.f};
    float charsSpeed{15.f};
    std::size_t numberOfGlyphs{0};
    bool muted{false};

    static const std::string MenuNameTag;
    static const std::string MainTextTag;
    static const std::string MoreTextImageTag;

    Bouncer moreTextImageBouncer;
    glm::vec2 moreTextImageOffsetPosition;

    AudioManager* audioManager{nullptr};
};
