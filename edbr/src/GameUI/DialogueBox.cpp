#include <edbr/GameUI/DialogueBox.h>

#include <edbr/AudioManager.h>
#include <edbr/Core/JsonFile.h>
#include <edbr/Graphics/GfxDevice.h>

#include <edbr/UI/ImageElement.h>
#include <edbr/UI/ListLayoutElement.h>
#include <edbr/UI/NineSliceElement.h>
#include <edbr/UI/RectElement.h>
#include <edbr/UI/TextElement.h>
#include <edbr/UI/UIRenderer.h>

const std::string DialogueBox::MenuNameTag{"menu_name"};
const std::string DialogueBox::MainTextTag{"main_text"};
const std::string DialogueBox::MoreTextImageTag{"more_text"};

void DialogueBox::init(GfxDevice& gfxDevice, AudioManager& audioManager)
{
    this->audioManager = &audioManager;

    bool ok = defaultFont.load(gfxDevice, "assets/fonts/JF-Dot-Kappa20B.ttf", 40, false);

    assert(ok && "font failed to load");
    createUI(gfxDevice);
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
    dialogueBoxUI->calculateLayout(screenSize);

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

        const auto glyphIdx = static_cast<std::size_t>(std::floor(numGlyphsToDraw) - 1);
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
                if (!muted) {
                    audioManager->playSound("assets/sounds/ui/text.wav");
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

        getMainTextElement().numGlyphsToDraw = (int)std::round(numGlyphsToDraw);
    }

    auto& moreTextImage = getMoreTextImageElement();
    moreTextImage.visible = isWholeTextDisplayed();
    if (moreTextImage.visible) {
        moreTextImageBouncer.update(dt);
        moreTextImage.offsetPosition =
            moreTextImageOffsetPosition + glm::vec2{0.f, moreTextImageBouncer.getOffset()};
    }
}

void DialogueBox::draw(SpriteRenderer& spriteRenderer) const
{
    ui::drawElement(spriteRenderer, *dialogueBoxUI);
}

void DialogueBox::setVisible(bool b)
{
    dialogueBoxUI->visible = b;
}

bool DialogueBox::isVisible() const
{
    return dialogueBoxUI->visible;
}

void DialogueBox::setText(const std::string t)
{
    text = std::move(t);
    getMainTextElement().text = text;

    // TODO: function to calculate number of glyphs
    numberOfGlyphs = 0;
    defaultFont.forEachGlyph(text, [this](const glm::vec2&, const glm::vec2&, const glm::vec2&) {
        ++numberOfGlyphs;
    });
}

void DialogueBox::setSpeakerName(const std::string t)
{
    getMenuNameTextElement().text = t;
}

void DialogueBox::createUI(GfxDevice& gfxDevice)
{
    ui::NineSliceStyle nsStyle;

    JsonFile file(std::filesystem::path{"assets/ui/nine_slice.json"});
    nsStyle.load(file.getLoader(), gfxDevice);

    dialogueBoxUI = std::make_unique<ui::Element>();
    dialogueBoxUI->relativePosition = {0.075f, 0.1f};
    dialogueBoxUI->autoSize = true;

    auto dialogueBox = std::make_unique<ui::NineSliceElement>(nsStyle);
    dialogueBox->autoSize = true;

    // main text
    auto mainTextElement = std::make_unique<ui::TextElement>(defaultFont, LinearColor::White());
    mainTextElement->tag = MainTextTag;
    {
        // calculate fixed size
        int maxNumCharsLine = 30;
        int maxLines = 4;
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

    // (16px, 8px) padding is added on both sides
    const auto fontHeight = defaultFont.lineSpacing;
    const auto padding = glm::vec2{
        std::round(fontHeight * 0.6f),
        std::round(fontHeight * 0.3f),
    };

    mainTextElement->offsetPosition = padding;
    mainTextElement->offsetSize = -padding * 2.f;
    mainTextElement->shadow = true;
    dialogueBox->addChild(std::move(mainTextElement));

    // menu name
    auto menuName =
        std::make_unique<ui::TextElement>(defaultFont, LinearColor::FromRGB(254, 214, 124));
    menuName->tag = MenuNameTag;
    menuName->offsetPosition = {std::round(fontHeight / 2.f), -fontHeight};
    menuName->shadow = true;
    dialogueBox->addChild(std::move(menuName));

    // more text image
    const auto moreTextImageId = gfxDevice.loadImageFromFile("assets/images/ui/more_text.png");
    const auto& moreTextImage = gfxDevice.getImage(moreTextImageId);
    auto moreTextImageElement = std::make_unique<ui::ImageElement>(moreTextImage);
    moreTextImageElement->tag = MoreTextImageTag;
    moreTextImageElement->origin = glm::vec2{1.f, 1.f};
    moreTextImageElement->relativePosition = glm::vec2{1.f, 1.f};
    // TODO: set based on image/border size?
    moreTextImageOffsetPosition = {-16.f, -12.f};
    moreTextImageElement->offsetPosition = moreTextImageOffsetPosition;
    dialogueBox->addChild(std::move(moreTextImageElement));

    dialogueBoxUI->addChild(std::move(dialogueBox));

    moreTextImageBouncer = Bouncer({
        .maxOffset = 2.f,
        .moveDuration = 0.5f,
        .tween = glm::quadraticEaseInOut<float>,
    });
}

ui::TextElement& DialogueBox::getMainTextElement()
{
    return dialogueBoxUI->findElement<ui::TextElement>(MainTextTag);
}

ui::TextElement& DialogueBox::getMenuNameTextElement()
{
    return dialogueBoxUI->findElement<ui::TextElement>(MenuNameTag);
}

ui::ImageElement& DialogueBox::getMoreTextImageElement()
{
    return dialogueBoxUI->findElement<ui::ImageElement>(MoreTextImageTag);
}

bool DialogueBox::isWholeTextDisplayed() const
{
    return static_cast<std::size_t>(numGlyphsToDraw) == numberOfGlyphs;
}
