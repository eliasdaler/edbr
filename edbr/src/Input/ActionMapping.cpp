#include <edbr/Input/ActionMapping.h>

#include <algorithm> // std::clamp
#include <cassert>

#include <edbr/Core/JsonFile.h>
#include <stdexcept>

void ActionMapping::loadActions(const std::filesystem::path& path)
{
    JsonFile file(path);
    if (!file.isGood()) {
        throw std::runtime_error(
            fmt::format("failed to load input actions from {}", path.string()));
    }
    const auto loader = file.getLoader();

    for (const auto& action : loader.getLoader("actions").asVectorOf<std::string>()) {
        initActionState(action);
    }
    for (const auto& axis : loader.getLoader("axes").asVectorOf<std::string>()) {
        initAxisState(axis);
    }
    for (const auto& [action, krLoader] : loader.getLoader("keyRepeat").getKeyValueMap()) {
        float startDelay{};
        krLoader.get("startDelay", startDelay);
        float repeatPeriod{};
        krLoader.get("repeatPeriod", repeatPeriod);

        setActionKeyRepeatable(action, startDelay, repeatPeriod);
    }
}

void ActionMapping::initActionState(const std::string& actionTagStr)
{
    auto id = static_cast<ActionTagHash>(actionHashes.size());
    actionHashes[actionTagStr] = id;
    (void)actionStates[id];
}

void ActionMapping::initAxisState(const std::string& axisTagStr)
{
    auto id = static_cast<ActionTagHash>(actionHashes.size());
    actionHashes[axisTagStr] = id;
    (void)axisStates[id];
}

bool ActionMapping::isPressed(ActionTagHash tag) const
{
    return actionStates.at(tag).isPressed;
}

bool ActionMapping::wasJustPressed(ActionTagHash tag) const
{
    if (!isMapped(tag)) {
        return false;
    }
    auto& state = actionStates.at(tag);
    bool justPressed = state.isPressed && !state.wasPressed;
    if (justPressed) {
        return true;
    }

    if (auto it = keyRepeatActions.find(tag); it != keyRepeatActions.end()) {
        return it->second.repeat;
    }
    return false;
}

bool ActionMapping::wasJustReleased(ActionTagHash tag) const
{
    if (!isMapped(tag)) {
        return false;
    }
    auto& state = actionStates.at(tag);
    return !state.isPressed && state.wasPressed;
}

bool ActionMapping::isHeld(ActionTagHash tag) const
{
    if (!isMapped(tag)) {
        return false;
    }
    auto& state = actionStates.at(tag);
    return state.isPressed && state.wasPressed;
}

float ActionMapping::getTimePressed(ActionTagHash tag) const
{
    if (!isMapped(tag)) {
        return 0.f;
    }
    return actionStates.at(tag).timePressed;
}

float ActionMapping::getAxisValue(ActionTagHash tag) const
{
    auto it = axisStates.find(tag);
    if (it != axisStates.end()) {
        return it->second.value;
    }
    fmt::print("Axis {} doesn't currently exist\n", tag);
    return 0.0f;
}

void ActionMapping::setActionPressed(ActionTagHash tag)
{
    if (!isMapped(tag)) {
        return;
    }

    auto& state = actionStates[tag];
    state.isPressed = true;
}

void ActionMapping::updateAxisState(ActionTagHash tag, float value)
{
    if (!isAxisMapped(tag)) {
        return;
    }

    auto& state = axisStates[tag];
    state.value += value;
}

void ActionMapping::onNewFrame()
{
    for (auto& [tag, actionState] : actionStates) {
        actionState.wasPressed = actionState.isPressed;
        // If no one calls setActionPressed in the frame, we
        // can assume that the action is not pressed anymore
        actionState.isPressed = false;
    }

    for (auto& [tag, axisState] : axisStates) {
        axisState.prevValue = axisState.value;
        axisState.value = 0.f;
    }
}

void ActionMapping::update(float dt)
{
    for (auto& [tag, state] : actionStates) {
        if (state.isPressed) {
            state.timePressed += dt;
        } else {
            state.timePressed = 0.f;
        }
    }

    for (auto& [tag, state] : keyRepeatActions) {
        bool isKeyPressed = isPressed(tag);
        if (!isKeyPressed) {
            state.delayPassed = false;
            state.repeat = false;
            state.pressValue = 0.f;
        } else {
            state.pressValue += dt;

            if (!state.delayPassed) {
                if (state.pressValue >= state.startDelay) {
                    state.pressValue -= state.startDelay;
                    state.delayPassed = true;
                }
            }

            if (state.delayPassed) {
                if (state.pressValue > state.repeatPeriod) {
                    state.pressValue -= state.repeatPeriod;
                    state.repeat = true;
                } else {
                    state.repeat = false;
                }
            }
        }
    }

    for (auto& [tag, axisState] : axisStates) {
        axisState.prevValue = axisState.value;
        axisState.value = std::clamp(axisState.value, -1.f, 1.f);
    }
}

void ActionMapping::resetState()
{
    for (auto& [tag, state] : actionStates) {
        state.isPressed = false;
        state.wasPressed = false;
        state.timePressed = 0.f;
    }

    for (auto& [tag, state] : axisStates) {
        state.value = 0.f;
        state.prevValue = 0.f;
    }
}

void ActionMapping::setActionKeyRepeatable(
    const std::string& tagStr,
    float startDelay,
    float keyRepeatPeriod)
{
    auto tag = getActionTagHash(tagStr);
    assert(tag != ACTION_NONE_HASH);
    assert(startDelay > 0.f);
    assert(keyRepeatPeriod > 0.f);

    KeyRepeatState krs;
    krs.startDelay = startDelay;
    krs.repeatPeriod = keyRepeatPeriod;
    keyRepeatActions[tag] = krs;
}

bool ActionMapping::isMapped(ActionTagHash tag) const
{
    return tag != ACTION_NONE_HASH && actionStates.find(tag) != actionStates.end();
}

bool ActionMapping::isAxisMapped(ActionTagHash tag) const
{
    return tag != ACTION_NONE_HASH && axisStates.find(tag) != axisStates.end();
}

ActionTagHash ActionMapping::getActionTagHash(const std::string& tagStr) const
{
    auto it = actionHashes.find(tagStr);
    if (it == actionHashes.end()) {
        throw std::runtime_error(
            fmt::format("ActionMapping: action '{}' was not registered", tagStr));
    }
    return it->second;
}
