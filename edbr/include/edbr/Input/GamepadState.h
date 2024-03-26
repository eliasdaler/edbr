#pragma once

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <SDL_gamecontroller.h>

union SDL_Event;

#include "ActionTagHash.h"
#include "ButtonState.h"

class ActionMapping;

using GamepadButton = SDL_GameControllerButton;
using GamepadAxis = SDL_GameControllerAxis;
using GamepadId = int;

class JsonDataLoader;
static const int MAX_CONTROLLERS = 16;

class GamepadState {
public:
    struct ActionAxisMapping {
        GamepadAxis axis;
        bool positive{false}; // if true, pusing axis in positive direction is a "press"
    };

public:
    GamepadState();
    void init();
    void cleanup();
    void loadMapping(const JsonDataLoader& loader, ActionMapping& actionMapping);

    void onNewFrame();
    void handleEvent(const SDL_Event& event, ActionMapping& actionMapping);
    void update(float dt, ActionMapping& actionMapping);
    void handleButtonEvent(const SDL_Event& event, ActionMapping& actionMapping);

    void addActionMapping(GamepadButton button, ActionTagHash tag);
    void addAxisActionMapping(ActionAxisMapping mapping, ActionTagHash tag);

    void addAxisMapping(GamepadAxis axis, ActionTagHash tag);
    void addButtonAxisMapping(GamepadButton key, ActionTagHash tag, float scale);

    GamepadId getId() const { return id; }

    void resetInput();

    float getAxisValue(GamepadAxis axis) const;
    void setAxisInverted(GamepadAxis axis, bool b);
    bool isConnected() const { return id != -1; }

private:
    void findConnectedGamepad();

    struct Axis {
        Axis();

        Axis(GamepadAxis axis, float deadZone, bool inverted);
        void reset();
        void setValue(std::int16_t rawValue);
        float getValue() const;
        void setInverted(bool b);

        // data members
        GamepadAxis axis;
        float deadZone;
        bool inverted;
        float value;
    };

    struct ButtonAxisBinding {
        ActionTagHash tag;
        float scale;
    };

    std::array<ButtonState, SDL_CONTROLLER_BUTTON_MAX> buttonStates;
    std::unordered_map<GamepadAxis, Axis> axes;

    std::array<SDL_GameController*, MAX_CONTROLLERS> handles;
    int id{-1}; // current id of gamepad being used
                // TODO: make possible to use multiple gamepads

    // bindings
    std::unordered_map<ActionTagHash, std::vector<ActionTagHash>> buttonActionBindings;
    std::unordered_map<ActionTagHash, std::vector<ActionAxisMapping>> axisActionBindings;
    // Axis bindings
    std::unordered_map<GamepadAxis, std::vector<ActionTagHash>> axisBindings;
    std::unordered_map<GamepadButton, ButtonAxisBinding> buttonAxisBindings;

    static const float DEAD_ZONE;
    static const float AXIS_PRESS_ZONE;
};
