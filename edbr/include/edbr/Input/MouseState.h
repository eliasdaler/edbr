#pragma once

#include <array>
#include <unordered_map>

#include "ActionTagHash.h"
#include "ButtonState.h"

#include <SDL_mouse.h>

#include <glm/vec2.hpp>

class ActionMapping;

union SDL_Event;
using SDLMouseButtonID = std::uint8_t;

class MouseState {
public:
    MouseState();

    void onNewFrame();
    void handleEvent(const SDL_Event& event, ActionMapping& actionMapping);

    void addActionMapping(SDLMouseButtonID button, ActionTagHash tag);

    const glm::ivec2& getPosition() const;
    glm::ivec2 getDelta() const;

    bool wasJustPressed(SDLMouseButtonID button) const;
    bool isHeld(SDLMouseButtonID button) const;
    bool wasJustReleased(SDLMouseButtonID button) const;

    void resetInput();

private:
    ActionTagHash getActionTag(SDLMouseButtonID button) const;

    glm::ivec2 position; // current frame position
    glm::ivec2 prevPosition; // previous frame position

    std::unordered_map<SDLMouseButtonID, ActionTagHash> mouseActionBindings;
    std::array<ButtonState, 16> mouseButtonStates;
};
