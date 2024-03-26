#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include "ActionTagHash.h"

class ActionMapping {
public:
    void loadActions(const std::filesystem::path& path);

    void initActionState(const std::string& actionTagStr);
    void initAxisState(const std::string& axisTagStr);

    // Functions used to get immediate state of action
    bool isPressed(ActionTagHash tag) const;
    bool wasJustPressed(ActionTagHash tag) const;
    bool wasJustReleased(ActionTagHash tag) const;
    bool isHeld(ActionTagHash tag) const;

    float getTimePressed(ActionTagHash tag) const;

    float getAxisValue(ActionTagHash tag) const;

    // functions below are used by InputManager to update actions and axes state
    void setActionPressed(ActionTagHash tag);
    void updateAxisState(ActionTagHash tag, float value);
    // functions used by InputManager to manage state of ActionMapping
    void onNewFrame();
    void update(float dt);
    void resetState();

    ActionTagHash getActionTagHash(const std::string& tagStr) const;

    void setActionKeyRepeatable(const std::string& tagStr, float startDelay, float keyRepeatPeriod);

private:
    bool isMapped(ActionTagHash tag) const;
    bool isAxisMapped(ActionTagHash tag) const;

    struct ActionState {
        float timePressed{0.f};
        bool isPressed{false};
        bool wasPressed{false};
    };

    struct AxisState {
        float value{0.f}; // scaled, from -1 to 1
        float prevValue{0.f};
    };

    std::unordered_map<ActionTagHash, ActionState> actionStates;
    std::unordered_map<ActionTagHash, AxisState> axisStates;

    std::unordered_map<std::string, ActionTagHash> actionHashes;

    struct KeyRepeatState {
        bool repeat{false}; // if true, wasJustPressed will trigger this frame
        float startDelay{0.f}; // delay between the press start and the first key repeat
        bool delayPassed{false}; // set to true after delay is exceeded
        float repeatPeriod{0.f}; // how fast the key repeat is triggered afterwards
        float pressValue{0.f}; // how long have passed since the press or last key repeat
    };
    std::unordered_map<ActionTagHash, KeyRepeatState> keyRepeatActions;
};
