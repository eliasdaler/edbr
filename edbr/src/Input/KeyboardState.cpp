#include <edbr/Input/KeyboardState.h>

#include <edbr/Input/ActionMapping.h>
#include <edbr/Input/InputStringMap.h>

#include <edbr/Core/JsonDataLoader.h>

#include <SDL_events.h>

void KeyboardState::loadMapping(const JsonDataLoader& loader, ActionMapping& actionMapping)
{
    for (auto& [tagStr, keysLoader] : loader.getLoader("action").getKeyValueMap()) {
        const auto tag = actionMapping.getActionTagHash(tagStr);
        for (const auto& keyStr : keysLoader.asVectorOf<std::string>()) {
            const auto scancode = toSDLScancode(keyStr);
            addActionMapping(scancode, tag);
        }
    }

    for (auto& mappingLoader : loader.getLoader("axisButton").getVector()) {
        std::string keyStr;
        mappingLoader.get("key", keyStr);

        std::string tagStr;
        mappingLoader.get("tag", tagStr);

        float scale{};
        mappingLoader.get("scale", scale);

        const auto scancode = toSDLScancode(keyStr);
        const auto tag = actionMapping.getActionTagHash(tagStr);

        addAxisMapping(scancode, tag, scale);
    }
}

void KeyboardState::onNewFrame()
{
    for (auto& keyState : keyStates) {
        keyState.onNewFrame();
    }
}

void KeyboardState::handleEvent(const SDL_Event& event, ActionMapping& actionMapping)
{
    const auto key = event.key.keysym.scancode;
    if (key == SDL_SCANCODE_UNKNOWN) {
        return;
    }

    const bool isPressed = (event.type == SDL_KEYDOWN);
    keyStates[key].pressed = isPressed;
}

void KeyboardState::update(float /*dt*/, ActionMapping& actionMapping)
{
    for (auto& [tag, keys] : keyActionBindings) {
        for (const auto& key : keys) {
            if (keyStates[key].pressed) {
                actionMapping.setActionPressed(tag);
            }
        }
    }

    for (auto& [key, bindings] : axisButtonBindings) {
        if (keyStates[key].pressed) {
            for (const auto& binding : bindings) {
                actionMapping.updateAxisState(binding.tag, binding.scale);
            }
        }
    }
}

void KeyboardState::addActionMapping(SDL_Scancode key, ActionTagHash tag)
{
    keyActionBindings[tag].push_back(key);
}

void KeyboardState::addAxisMapping(SDL_Scancode key, ActionTagHash tag, float scale)
{
    axisButtonBindings[key].push_back(ButtonAxisBinding{tag, scale});
}

bool KeyboardState::wasJustPressed(SDL_Scancode key) const
{
    return keyStates[key].wasJustPressed();
}

bool KeyboardState::wasJustReleased(SDL_Scancode key) const
{
    return keyStates[key].wasJustReleased();
}

bool KeyboardState::isPressed(SDL_Scancode key) const
{
    return keyStates[key].isPressed();
}

bool KeyboardState::isHeld(SDL_Scancode key) const
{
    return keyStates[key].isHeld();
}

void KeyboardState::resetInput()
{
    for (auto& keyState : keyStates) {
        keyState.reset();
    }
}
