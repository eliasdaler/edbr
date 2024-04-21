#pragma once

#include <string>

#include <edbr/UI/Element.h>

#include <edbr/Graphics/Color.h>

namespace ui
{

struct ButtonStyle;
class TextElement;

class ButtonElement : public Element {
public:
    ButtonElement(const ButtonStyle& bs, std::string text, std::function<void()> onButtonPress);

    ui::Element& getFrameElement();
    const ui::Element& getFrameElement() const;

    ui::TextElement& getTextElement();
    const ui::TextElement& getTextElement() const;

    void calculateChildrenSizes() override;

    void onSelected() override;
    void onDeselected() override;

    void onEnabled() override;
    void onDisabled() override;

    LinearColor normalColor;
    LinearColor selectedColor;
    LinearColor disabledColor;

private:
};

} // end of namespace ui
