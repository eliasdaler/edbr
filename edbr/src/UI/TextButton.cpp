#include <edbr/UI/TextButton.h>

#include <edbr/UI/NineSlice.h>

namespace ui
{

TextButton::TextButton(
    const ui::NineSliceStyle& nineSliceStyle,
    const Font& font,
    const std::string& label,
    const LinearColor& labelColor) :
    TextButton(
        std::make_unique<NineSliceElement>(nineSliceStyle, glm::vec2{}),
        std::make_unique<TextLabel>(label, font, labelColor))
{}

TextButton::TextButton(
    std::unique_ptr<NineSliceElement> nineSlice,
    std::unique_ptr<TextLabel> label)
{
    label->setPadding({12.f, 12.f, 0.f, 8.f}); // TODO: make adjustable
    nineSlice->addChild(std::move(label));

    nineSlice->setAutomaticSizing(AutomaticSizing::XY);
    addChild(std::move(nineSlice));
}

NineSliceElement& TextButton::getNineSlice()
{
    return const_cast<NineSliceElement&>(static_cast<const TextButton&>(*this).getNineSlice());
}

TextLabel& TextButton::getLabel()
{
    return const_cast<TextLabel&>(static_cast<const TextButton&>(*this).getLabel());
}

glm::vec2 TextButton::getSizeImpl() const
{
    return getNineSlice().getSize();
}

const NineSliceElement& TextButton::getNineSlice() const
{
    assert(children.size() == 1);
    return static_cast<NineSliceElement&>(*children[0]);
}

const TextLabel& TextButton::getLabel() const
{
    auto& ns = getNineSlice();
    assert(ns.getChildren().size() == 1);
    return static_cast<TextLabel&>(*ns.getChildren()[0]);
}

math::FloatRect TextButton::getBoundingBox() const
{
    return getNineSlice().getBoundingBox();
}

void TextButton::processMouseEvent(const glm::vec2& mouseRelPos, bool leftMousePressed)
{
    const auto bb = getBoundingBox();
    auto& label = getLabel();
    if (bb.contains(mouseRelPos)) {
        if (leftMousePressed) {
            // pressed
            label.setColor(LinearColor::FromRGB(254, 214, 124));
        } else {
            // howevered
            label.setColor(LinearColor::FromRGB(254, 174, 52));
        }
    } else {
        // default
        label.setColor(LinearColor::FromRGB(255, 255, 255));
    }
}

} // end of namespace ui
