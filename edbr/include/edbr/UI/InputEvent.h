#pragma once

#include <functional>

namespace ui
{
struct Element;

enum class ActionState { Pressed, Released, Held };

enum class ActionType {
    Confirm, // e.g. "A" button
    Back, // e.g. "B" button
    // cursor / focus movement
    Up,
    Down,
    Left,
    Right
};

struct InputEvent {
    ActionState actionState;
    ActionType actionType;
    float timeHeld{0.f}; // only for ActionState::Held

    bool pressedConfirm() const
    {
        return actionState == ActionState::Pressed && actionType == ActionType::Confirm;
    }
};

// Should return true if input action was handled
using InputHandlerType = std::function<bool(Element&, const InputEvent&)>;

InputHandlerType OnConfirmPressedHandler(std::function<void()> callback);

}
