#include <edbr/Input/ButtonState.h>

ButtonState::ButtonState() : pressed(false), wasPressed(false)
{}

void ButtonState::onNewFrame()
{
    wasPressed = pressed;
}

void ButtonState::reset()
{
    wasPressed = false;
    pressed = false;
}

bool ButtonState::wasJustPressed() const
{
    return !wasPressed && pressed;
}

bool ButtonState::wasJustReleased() const
{
    return wasPressed && !pressed;
}

bool ButtonState::isPressed() const
{
    return pressed;
}

bool ButtonState::isHeld() const
{
    return wasPressed && pressed;
}
