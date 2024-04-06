#include <edbr/UI/TextButton.h>

#include <edbr/UI/NineSlice.h>

namespace ui
{

TextButton::TextButton(
    const ui::NineSliceStyle& nineSliceStyle,
    const Font& font,
    const std::string& label) :
    TextButton(
        std::make_unique<NineSliceElement>(nineSliceStyle, glm::vec2{}),
        std::make_unique<TextLabel>(label, font))
{}

TextButton::TextButton(
    std::unique_ptr<NineSliceElement> nineSlice,
    std::unique_ptr<TextLabel> label)
{
    label->setPadding({12.f, 12.f, 2.f, 8.f}); // TODO: make adjustable
    nineSlice->addChild(std::move(label));

    nineSlice->setAutomaticSizing(AutomaticSizing::XY);
    addChild(std::move(nineSlice));
}

const NineSliceElement& TextButton::getNineSlice() const
{
    assert(children.size() == 1);
    return static_cast<const NineSliceElement&>(*children[0]);
}

const TextLabel& TextButton::getLabel() const
{
    auto& ns = getNineSlice();
    assert(ns.getChildren().size() == 1);
    return static_cast<const TextLabel&>(*ns.getChildren()[0]);
}

glm::vec2 TextButton::getContentSize() const
{
    return getNineSlice().getSize();
}

} // end of namespace ui
