#include <edbr/Input/MouseState.h>

#include <edbr/Input/ActionMapping.h>

#include <SDL_events.h>

MouseState::MouseState() : prevPosition(getPosition())
{}

void MouseState::onNewFrame()
{
    prevPosition = position;
    SDL_GetMouseState(&position.x, &position.y);
    for (auto& mouseButtonState : mouseButtonStates) {
        mouseButtonState.onNewFrame();
    }
}

void MouseState::handleEvent(const SDL_Event& event, ActionMapping& actionMapping)
{
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
        const bool isPressed = (event.type == SDL_MOUSEBUTTONDOWN);
        const auto button = (SDLMouseButtonID)event.button.button;
        auto& buttonState = mouseButtonStates[button];

        buttonState.pressed = isPressed;

        // handle button press
        auto actionTag = getActionTag(button);
        if (actionTag != ACTION_NONE_HASH && isPressed) {
            actionMapping.setActionPressed(actionTag);
        }
        break;
    }
    default:
        break;
    }
}

void MouseState::addActionMapping(SDLMouseButtonID button, ActionTagHash tag)
{
    mouseActionBindings.emplace(button, tag);
}

bool MouseState::wasJustPressed(SDLMouseButtonID button) const
{
    return mouseButtonStates[button].wasJustPressed();
}

bool MouseState::isHeld(SDLMouseButtonID button) const
{
    return mouseButtonStates[button].isHeld();
}

bool MouseState::wasJustReleased(SDLMouseButtonID button) const
{
    return mouseButtonStates[button].wasJustReleased();
}

void MouseState::resetInput()
{
    for (auto& mouseButtonState : mouseButtonStates) {
        mouseButtonState.reset();
    }
}

ActionTagHash MouseState::getActionTag(SDLMouseButtonID button) const
{
    auto it = mouseActionBindings.find(button);
    return (it != mouseActionBindings.end()) ? it->second : ACTION_NONE_HASH;
}

const glm::ivec2& MouseState::getPosition() const
{
    return position;
}

glm::ivec2 MouseState::getDelta() const
{
    return position - prevPosition;
}
