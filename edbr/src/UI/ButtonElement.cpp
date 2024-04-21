#include <edbr/UI/ButtonElement.h>

#include <edbr/UI/NineSliceElement.h>
#include <edbr/UI/Style.h>
#include <edbr/UI/TextElement.h>

namespace ui
{

ButtonElement::ButtonElement(
    const ButtonStyle& bs,
    std::string text,
    std::function<void()> onButtonPress) :
    normalColor(bs.normalColor), selectedColor(bs.selectedColor), disabledColor(bs.disabledColor)
{
    autoSize = true;
    cursorSelectionOffset = bs.cursorOffset;

    auto textElement = std::make_unique<ui::TextElement>(std::move(text), bs.font, bs.normalColor);
    if (bs.textAlignment == ButtonStyle::TextAlignment::Center) {
        textElement->origin = {0.5f, 0.5f};
        textElement->relativePosition = {0.5f, 0.5f};
        // padding is added on both sides
        textElement->offsetSize = -bs.padding * 2.f;
    } else {
        textElement->origin = {0.f, 0.5f};
        textElement->relativePosition = {0.f, 0.5f};
        textElement->offsetPosition = {bs.padding.x, 0.f};
        textElement->offsetSize = -bs.padding * 2.f;
    }

    std::unique_ptr<ui::Element> frameElement;
    if (bs.nineSliceStyle) {
        frameElement = std::make_unique<NineSliceElement>(*bs.nineSliceStyle);
    } else {
        frameElement = std::make_unique<ui::Element>();
    }

    frameElement->autoSize = true;
    frameElement->addChild(std::move(textElement));

    addChild(std::move(frameElement));

    inputHandler = ui::OnConfirmPressedHandler(onButtonPress);
}

ui::Element& ButtonElement::getFrameElement()
{
    assert(children.size() == 1);
    return *children.at(0);
}

const ui::Element& ButtonElement::getFrameElement() const
{
    assert(children.size() == 1);
    return *children.at(0);
}

ui::TextElement& ButtonElement::getTextElement()
{
    auto& frame = getFrameElement();
    assert(frame.children.size() == 1);
    return static_cast<ui::TextElement&>(*frame.children.at(0));
}

const ui::TextElement& ButtonElement::getTextElement() const
{
    auto& frame = getFrameElement();
    assert(frame.children.size() == 1);
    return static_cast<ui::TextElement&>(*frame.children.at(0));
}

void ButtonElement::calculateChildrenSizes()
{
    // frame inherits button's size (possibly set by ListLayout, for example)
    auto& frame = getFrameElement();
    frame.absoluteSize = absoluteSize;
    frame.calculateChildrenSizes();
}

void ButtonElement::onSelected()
{
    Element::onSelected();
    if (enabled) {
        auto& textElement = getTextElement();
        textElement.color = selectedColor;
    }
}

void ButtonElement::onDeselected()
{
    Element::onDeselected();
    if (enabled) {
        auto& textElement = getTextElement();
        textElement.color = normalColor;
    }
}

void ButtonElement::onEnabled()
{
    Element::onEnabled();
    auto& textElement = getTextElement();
    textElement.color = selected ? selectedColor : normalColor;
}

void ButtonElement::onDisabled()
{
    Element::onDisabled();
    auto& textElement = getTextElement();
    textElement.color = disabledColor;
}

} // end of namespace ui
