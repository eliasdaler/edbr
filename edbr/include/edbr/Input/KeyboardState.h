#pragma once

#include <array>
#include <unordered_map>
#include <vector>

#include <SDL_scancode.h>

#include "ActionTagHash.h"
#include "ButtonState.h"

union SDL_Event;

class ActionMapping;
class JsonDataLoader;

class KeyboardState {
public:
    void loadMapping(const JsonDataLoader& loader, ActionMapping& actionMapping);

    void onNewFrame();
    void handleEvent(const SDL_Event& event, ActionMapping& actionMapping);
    void update(float dt, ActionMapping& actionMapping);

    void addActionMapping(SDL_Scancode key, ActionTagHash tag);
    void addAxisMapping(SDL_Scancode key, ActionTagHash tag, float scale);

    bool wasJustPressed(SDL_Scancode key) const;
    bool wasJustReleased(SDL_Scancode key) const;
    bool isPressed(SDL_Scancode key) const;
    bool isHeld(SDL_Scancode key) const;

    void resetInput();

private:
    struct ButtonAxisBinding {
        ActionTagHash tag;
        float scale;
    };

    // state
    std::array<ButtonState, SDL_NUM_SCANCODES> keyStates;

    // bindings
    std::unordered_map<ActionTagHash, std::vector<SDL_Scancode>> keyActionBindings;
    std::unordered_map<SDL_Scancode, std::vector<ButtonAxisBinding>> axisButtonBindings;
};
