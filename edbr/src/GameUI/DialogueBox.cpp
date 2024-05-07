#include <edbr/GameUI/DialogueBox.h>

#include <edbr/Audio/AudioManager.h>
#include <edbr/Core/JsonFile.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Input/ActionMapping.h>

#include <edbr/UI/ButtonElement.h>
#include <edbr/UI/ImageElement.h>
#include <edbr/UI/ListLayoutElement.h>
#include <edbr/UI/NineSliceElement.h>
#include <edbr/UI/RectElement.h>
#include <edbr/UI/TextElement.h>
#include <edbr/UI/UIRenderer.h>

#include <edbr/DevTools/ImGuiPropertyTable.h>
#include <imgui.h>

const std::string DialogueBox::DialogueBoxMenuTag{"dialogue_box"};
const std::string DialogueBox::MenuNameTag{"menu_name"};
const std::string DialogueBox::MainTextTag{"main_text"};
const std::string DialogueBox::MoreTextImageTag{"more_text"};
const std::string DialogueBox::Choice1Tag{"choice1"};
const std::string DialogueBox::Choice2Tag{"choice2"};

void DialogueBoxStyle::load(const JsonDataLoader& loader, GfxDevice& gfxDevice)
{
    nineSliceStyle.load(loader.getLoader("nineSliceStyle"), gfxDevice);
    loader.get("maxNumCharsLine", maxNumCharsLine);
    loader.get("maxLines", maxLines);
    mainTextFontStyle.load(loader.getLoader("mainTextFont"));
    loader.get("moreTextImage", moreTextImagePath);
    choiceButtonStyle.load(loader.getLoader("choiceButtonStyle"));

    const auto& soundsLoader = loader.getLoader("sounds");
    soundsLoader.get("showChoices", showChoicesSoundPath);
    soundsLoader.get("choiceSelect", choiceSelectSoundPath);
    soundsLoader.get("skipText", skipTextSoundPath);
}

void DialogueBox::init(
    const DialogueBoxStyle& dbStyle,
    GfxDevice& gfxDevice,
    AudioManager& audioManager)
{
    this->audioManager = &audioManager;

    bool ok = defaultFont.load(
        gfxDevice,
        dbStyle.mainTextFontStyle.path,
        dbStyle.mainTextFontStyle.size,
        dbStyle.mainTextFontStyle.antialiasing);
    assert(ok && "font failed to load");

    showChoicesSoundPath = dbStyle.showChoicesSoundPath;
    choiceSelectSoundPath = dbStyle.choiceSelectSoundPath;
    skipTextSoundPath = dbStyle.skipTextSoundPath;

    createUI(dbStyle, gfxDevice);
}

void DialogueBox::handleInput(const ActionMapping& actionMapping)
{
    static const auto interactAction = actionMapping.getActionTagHash("PrimaryAction");
    if (actionMapping.wasJustPressed(interactAction)) {
        if (!isWholeTextDisplayed()) {
            if (!skipTextSoundPath.empty()) {
                audioManager->playSound(skipTextSoundPath);
            }
            displayWholeText();
        } else if (!hasChoices) {
            advanceTextPressed = true;
        }
    }
}

namespace
{
bool isPunctuation(char c)
{
    return c == '!' || c == '.' || c == '?' || c == ',';
}

}

void DialogueBox::update(const glm::vec2& screenSize, float dt)
{
    if (!isWholeTextDisplayed()) {
        if (currentTextDelay != 0.f) {
            delayValue += dt;
            if (delayValue > currentTextDelay) {
                delayValue = 0.f;
                currentTextDelay = 0.f;
                textSoundTriggerValue = textSoundTriggerSpeed;
            } else {
                return;
            }
        }

        numGlyphsToDraw += charsSpeed * dt;
        if (numGlyphsToDraw >= numberOfGlyphs) {
            numGlyphsToDraw = static_cast<float>(numberOfGlyphs);
        }

        const auto glyphIdx =
            static_cast<std::size_t>(std::max(std::floor(numGlyphsToDraw), 1.f) - 1);
        // TODO: should do getGlyph(text, charIdx) here
        const auto lastLetter = glyphIdx > 0 ? text[glyphIdx] : '\0';
        bool punctuation = isPunctuation(lastLetter);
        if (!punctuation) {
            // don't trigger text sound when punctuation is displayed

            textSoundTriggerValue += dt;
            if (wasPunctuation) {
                // trigger sound immediately if punctuation was previously displayed
                textSoundTriggerValue = textSoundTriggerSpeed;
            }
            if (textSoundTriggerValue >= textSoundTriggerSpeed) {
                textSoundTriggerValue -= textSoundTriggerSpeed;
                if (!muted && !voiceSound.empty()) {
                    audioManager->playSound(voiceSound);
                }
            }
        }

        // delay on punctuation
        if (punctuation && !delaysDone.contains(glyphIdx)) {
            currentTextDelay = punctuationDelayTime;
            // the delay should only be triggered once
            delaysDone.insert(glyphIdx);
        }
        wasPunctuation = punctuation;
    }
    getMainTextElement().numGlyphsToDraw = (int)std::round(numGlyphsToDraw);

    if (isWholeTextDisplayed() && hasChoices && !choicesDisplayed) {
        setChoicesDisplayed(true);
        if (!showChoicesSoundPath.empty()) {
            audioManager->playSound(showChoicesSoundPath);
        }
    }

    if (isWholeTextDisplayed() && !choicesDisplayed) {
        auto& moreTextImage = getMoreTextImageElement();
        moreTextImage.visible = true;

        moreTextImageBouncer.update(dt);
        moreTextImage.offsetPosition =
            moreTextImageOffsetPosition + glm::vec2{0.f, moreTextImageBouncer.getOffset()};
    } else {
        auto& moreTextImage = getMoreTextImageElement();
        moreTextImage.visible = false;
    }
}

void DialogueBox::draw(SpriteRenderer& spriteRenderer) const
{
    ui::drawElement(spriteRenderer, *dialogueBoxUI);
}

void DialogueBox::setText(std::string t)
{
    advanceTextPressed = false;

    text = std::move(t);
    getMainTextElement().text = text;

    // TODO: function to calculate number of glyphs
    getMainTextElement().numGlyphsToDraw = (int)std::round(numGlyphsToDraw);
    defaultFont.forEachGlyph(text, [this](const glm::vec2&, const glm::vec2&, const glm::vec2&) {
        ++numberOfGlyphs;
    });
}

void DialogueBox::resetState()
{
    numberOfGlyphs = 0;
    numGlyphsToDraw = 0;
    getMoreTextImageElement().visible = false;

    // clear all text inside to be extra safe
    setText("");
    setSpeakerName("");

    voiceSound = defaultVoiceSound;
    textSoundTriggerSpeed = 0.08f;

    // handle choices
    hasChoices = false;
    setChoicesDisplayed(false);
    if (choiceSelectionIndex != -1) {
        lastChoiceSelection = static_cast<std::size_t>(choiceSelectionIndex);
    }
    choiceSelectionIndex = -1;
    for (std::size_t index = 0; index < MaxChoices; ++index) {
        getChoiceButton(index).getTextElement().text.clear();
    }
}

void DialogueBox::setSpeakerName(const std::string t)
{
    getMenuNameTextElement().text = t;
}

void DialogueBox::createUI(const DialogueBoxStyle& dbStyle, GfxDevice& gfxDevice)
{
    dialogueBoxUI = std::make_unique<ui::Element>();
    dialogueBoxUI->relativePosition = {0.075f, 0.1f};
    dialogueBoxUI->autoSize = true;

    auto dialogueBox = std::make_unique<ui::NineSliceElement>(dbStyle.nineSliceStyle);
    dialogueBox->autoSize = true;

    const auto fontHeight = defaultFont.lineSpacing;
    { // main text
        auto mainTextElement = std::make_unique<ui::TextElement>(defaultFont, LinearColor::White());
        mainTextElement->tag = MainTextTag;
        { // calculate fixed max size
            int maxNumCharsLine = dbStyle.maxNumCharsLine;
            int maxLines = dbStyle.maxLines;
            std::string fixedSizeString{};
            fixedSizeString.reserve(maxNumCharsLine * maxLines + maxLines);
            for (int y = 0; y < maxLines; ++y) {
                for (int x = 0; x < maxNumCharsLine; ++x) {
                    fixedSizeString += "W";
                }
                if (y != maxLines - 1) {
                    fixedSizeString += "\n";
                }
            }
            mainTextElement->setFixedSize(fixedSizeString);
        }

        // some padding is added on both sides
        const auto padding = glm::vec2{
            std::round(fontHeight * 0.6f),
            std::round(fontHeight * 0.3f),
        };

        mainTextElement->offsetPosition = padding;
        mainTextElement->offsetSize = -padding * 2.f;
        mainTextElement->shadow = true;
        dialogueBox->addChild(std::move(mainTextElement));
    }

    { // choices
        auto listElement = std::make_unique<ui::ListLayoutElement>();
        listElement->direction = ui::ListLayoutElement::Direction::Vertical;
        listElement->autoSize = true;
        // place on bottom-left of the dialogue box
        listElement->origin = {0.f, 1.f};
        listElement->relativePosition = {0.f, 1.f};
        // TODO: don't hardcode in pixels?
        listElement->offsetPosition = {32.f, -8.f};
        listElement->offsetSize = {-64.f, -16.f};

        for (int i = 0; i < 2; ++i) {
            auto choiceButton = std::make_unique<
                ui::ButtonElement>(dbStyle.choiceButtonStyle, defaultFont, "CHOICE", [this, i]() {
                if (!choiceSelectSoundPath.empty()) {
                    audioManager->playSound(choiceSelectSoundPath);
                }
                choiceSelectionIndex = i;
            });
            choiceButton->tag = (i == 0) ? Choice1Tag : Choice2Tag;
            choiceButton->visible = false;
            listElement->addChild(std::move(choiceButton));
        }

        // manage cursor
        for (std::size_t i = 0; i < listElement->children.size(); ++i) {
            const auto prevIndex = (i == 0) ? listElement->children.size() - 1 : i - 1;
            const auto nextIndex = (i == listElement->children.size() - 1) ? 0 : i + 1;

            auto& button = *listElement->children[i];
            button.cursorUpElement = listElement->children[prevIndex].get();
            button.cursorDownElement = listElement->children[nextIndex].get();
        }

        dialogueBox->addChild(std::move(listElement));
    }

    { // menu/speaker name
        auto menuName =
            std::make_unique<ui::TextElement>(defaultFont, LinearColor::FromRGB(254, 214, 124));
        menuName->tag = MenuNameTag;
        menuName->offsetPosition = {std::round(fontHeight / 2.f), -fontHeight};
        menuName->shadow = true;
        dialogueBox->addChild(std::move(menuName));
    }

    { // more text image
        const auto moreTextImageId = gfxDevice.loadImageFromFile(dbStyle.moreTextImagePath);
        const auto& moreTextImage = gfxDevice.getImage(moreTextImageId);
        auto moreTextImageElement = std::make_unique<ui::ImageElement>(moreTextImage);
        moreTextImageElement->tag = MoreTextImageTag;
        moreTextImageElement->origin = glm::vec2{1.f, 1.f};
        moreTextImageElement->relativePosition = glm::vec2{1.f, 1.f};
        // TODO: set based on image/border size?
        moreTextImageOffsetPosition = {-16.f, -12.f};
        moreTextImageElement->offsetPosition = moreTextImageOffsetPosition;
        moreTextImageElement->visible = false;
        dialogueBox->addChild(std::move(moreTextImageElement));

        moreTextImageBouncer = Bouncer({
            .maxOffset = 2.f,
            .moveDuration = 0.5f,
            .tween = glm::quadraticEaseInOut<float>,
        });
    }

    // finish setup
    dialogueBoxUI->addChild(std::move(dialogueBox));
    dialogueBoxUI->cursorSelectionStartElement = dialogueBoxUI->tryFindElement(Choice1Tag);
    dialogueBoxUI->tag = DialogueBoxMenuTag;
}

ui::TextElement& DialogueBox::getMainTextElement()
{
    return dialogueBoxUI->findElement<ui::TextElement>(MainTextTag);
}

ui::TextElement& DialogueBox::getMenuNameTextElement()
{
    return dialogueBoxUI->findElement<ui::TextElement>(MenuNameTag);
}

ui::ButtonElement& DialogueBox::getChoiceButton(std::size_t index)
{
    assert(index < MaxChoices);
    const auto& tag = (index == 0) ? Choice1Tag : Choice2Tag;
    return dialogueBoxUI->findElement<ui::ButtonElement>(tag);
}

ui::ImageElement& DialogueBox::getMoreTextImageElement()
{
    return dialogueBoxUI->findElement<ui::ImageElement>(MoreTextImageTag);
}

void DialogueBox::displayWholeText()
{
    numGlyphsToDraw = numberOfGlyphs;
}

bool DialogueBox::isWholeTextDisplayed() const
{
    return static_cast<std::size_t>(numGlyphsToDraw) == numberOfGlyphs;
}

void DialogueBox::setChoicesDisplayed(bool b)
{
    choicesDisplayed = b;
    for (std::size_t index = 0; index < MaxChoices; ++index) {
        getChoiceButton(index).visible = b;
    }
}

void DialogueBox::setChoiceText(std::size_t index, std::string choice)
{
    hasChoices = true;
    getChoiceButton(index).getTextElement().text = std::move(choice);
}

bool DialogueBox::wantsClose() const
{
    return wantsTextAdvance() || madeChoiceSelection();
}

bool DialogueBox::madeChoiceSelection() const
{
    return hasChoices && choicesDisplayed && choiceSelectionIndex != -1;
}

std::size_t DialogueBox::getLastChoiceSelectionIndex() const
{
    return lastChoiceSelection;
}

void DialogueBox::displayDebugInfo()
{
    using namespace devtools;
    BeginPropertyTable();
    DisplayProperty("numberOfGlyphs", numberOfGlyphs);
    DisplayProperty("numGlyphsToDraw", numGlyphsToDraw);
    DisplayProperty("muted", muted);
    DisplayProperty("hasChoices", hasChoices);
    DisplayProperty("choicesDisplayed", choicesDisplayed);
    DisplayProperty("choiceSelectionIndex", choiceSelectionIndex);
    DisplayProperty("lastChoiceSelection", lastChoiceSelection);
    EndPropertyTable();
}

void DialogueBox::setDefaultVoiceSound(const std::string& soundName)
{
    defaultVoiceSound = soundName;
    voiceSound = defaultVoiceSound;
}

void DialogueBox::setTempVoiceSound(const std::string& soundName)
{
    voiceSound = soundName;
    textSoundTriggerSpeed = 0.15f;
}
