#pragma once

#include <memory>

#include <edbr/Graphics/Bouncer.h>
#include <edbr/Graphics/Font.h>
#include <edbr/UI/Element.h>
#include <edbr/UI/Style.h>

class GfxDevice;
class SpriteRenderer;

namespace ui
{
class TextElement;
class ImageElement;
class ButtonElement;
}

class IAudioManager;
class ActionMapping;

struct DialogueBoxStyle {
    void load(const JsonDataLoader& loader, GfxDevice& gfxDevice);

    ui::ElementPositionAndSize positionAndSize;
    ui::NineSliceStyle nineSliceStyle;

    // main text
    ui::FontStyle mainTextFontStyle;
    LinearColor mainTextColor;
    bool mainTextShadow{false};
    glm::vec2 mainTextPadding;
    int mainTextMaxNumCharsLine{};
    int mainTextMaxLines{};
    ui::ElementPositionAndSize mainTextPositionAndSize;

    // typewriter
    float charsDisplaySpeed{15.f};
    float letterSoundSpeed{0.08f};
    float punctuationDelay{0.1f};

    // speaker name text
    ui::FontStyle speakerNameTextFontStyle;
    LinearColor speakerNameTextColor;
    glm::vec2 speakerNameTextOffset;
    bool speakerNameTextShadow{false};
    ui::ElementPositionAndSize speakerNamePositionAndSize;

    // choice buttons
    ui::ButtonStyle choiceButtonStyle;
    glm::vec2 choiceButtonsPadding;

    // more text image
    std::filesystem::path moreTextImagePath;
    glm::vec2 moreTextImageOffset; // offset from bottom right corner of dialogue box
    Bouncer::Params moreTextImageBouncerParams;

    // sounds
    std::filesystem::path defaultLetterSoundPath;
    std::filesystem::path choiceSelectSoundPath;
    std::filesystem::path showChoicesSoundPath;
    std::filesystem::path skipTextSoundPath;
};

class DialogueBox {
public:
    void init(const DialogueBoxStyle& dbStyle, GfxDevice& gfxDevice, IAudioManager& audioManager);

    void handleInput(const ActionMapping& actionMapping);

    void update(float dt);

    void draw(SpriteRenderer& spriteRenderer) const;

    void setText(std::string t);
    void setSpeakerName(std::string t);
    void setChoiceText(std::size_t index, std::string choice);

    // should be called after closing the dialogue box
    void resetState();
    void displayWholeText();

    bool isWholeTextDisplayed() const;
    bool wantsClose() const;
    bool wantsTextAdvance() const { return advanceTextPressed; }
    bool madeChoiceSelection() const;
    std::size_t getLastChoiceSelectionIndex() const;

    ui::Element& getRootElement() { return *dialogueBoxUI; }

    void displayDebugInfo();

    static const std::string DialogueBoxMenuTag;
    static const std::size_t MaxChoices = 2;

    void setTempVoiceSound(const std::string& soundName, float letterSoundSpeed);

private:
    void createUI(const DialogueBoxStyle& dbStyle, GfxDevice& gfxDevice);
    ui::TextElement& getMainTextElement();
    ui::TextElement& getMenuNameTextElement();
    ui::ButtonElement& getChoiceButton(std::size_t index);
    ui::ImageElement& getMoreTextImageElement();

    void setChoicesDisplayed(bool b);

    Font defaultFont;
    Font speakerNameFont;
    std::unique_ptr<ui::Element> dialogueBoxUI;

    std::string text;

    float defaultLetterSoundSpeed{0.08f};
    float letterSoundSpeed{0.08f};
    float textSoundTriggerValue{0.f};

    float currentTextDelay{0.f};
    float delayValue{0.f};

    float punctuationDelayTime{0.1f};
    bool wasPunctuation{false};
    std::unordered_set<std::size_t> delaysDone;

    std::size_t numberOfGlyphs{0};
    float numGlyphsToDraw{0.f};
    float charsSpeed{15.f};
    bool muted{false};
    std::string voiceSound;
    std::string defaultVoiceSound;

    bool advanceTextPressed{false};
    bool hasChoices{false};
    bool choicesDisplayed{false};
    int choiceSelectionIndex{-1}; // if -1 - no selection yet made
    std::size_t lastChoiceSelection{};

    static const std::string MenuNameTag;
    static const std::string MainTextTag;
    static const std::string MoreTextImageTag;
    static const std::string Choice1Tag;
    static const std::string Choice2Tag;

    const int NumChoices = 2;

    Bouncer moreTextImageBouncer;
    glm::vec2 moreTextImageOffsetPosition;

    // DI
    IAudioManager* audioManager{nullptr};

    // sounds
    std::filesystem::path showChoicesSoundPath;
    std::filesystem::path choiceSelectSoundPath;
    std::filesystem::path skipTextSoundPath;
};
