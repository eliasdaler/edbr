#pragma once

#pragma once

#include <edbr/Math/Rect.h>
#include <edbr/UI/Element.h>
#include <edbr/UI/NineSliceElement.h>
#include <edbr/UI/TextLabel.h>

struct Font;

namespace ui
{

class TextButton : public Element {
public:
    struct Padding {
        float left{0.f};
        float right{0.f};
        float top{0.f};
        float bottom{0.f};
    };

public:
    TextButton(
        const ui::NineSliceStyle& nineSliceStyle,
        const Font& font,
        const std::string& label,
        const LinearColor& labelColor = LinearColor::Black());
    TextButton(std::unique_ptr<NineSliceElement> nineSlice, std::unique_ptr<TextLabel> label);

    NineSliceElement& getNineSlice();
    TextLabel& getLabel();

    const NineSliceElement& getNineSlice() const;
    const TextLabel& getLabel() const;

    glm::vec2 getSizeImpl() const override;
    math::FloatRect getBoundingBox() const override;

    void processMouseEvent(const glm::vec2& mouseRelPos, bool leftMousePressed) override;

private:
};

} // end of namespace ui
