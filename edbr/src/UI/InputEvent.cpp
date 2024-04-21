#include <edbr/UI/InputEvent.h>

#include <edbr/UI/Element.h>

namespace ui
{
InputHandlerType OnConfirmPressedHandler(std::function<void()> callback)
{
    return [callback](const Element& element, const ui::InputEvent& event) {
        if (!element.enabled) {
            return false;
        }
        if (event.pressedConfirm()) {
            callback();
            return true;
        }
        return false;
    };
}

}
