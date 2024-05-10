#include <edbr/Input/InputManager.h>

#include <edbr/Core/JsonFile.h>

#include <SDL_events.h>
#include <stdexcept>

#include <fmt/format.h>

InputManager::InputManager()
{
    usingGamepad = false;
    gamepad.init();
    if (gamepad.isConnected()) {
        usingGamepad = true;
    }
}

void InputManager::loadMapping(
    const std::filesystem::path& inputActionsPath,
    const std::filesystem::path& inputMappingPath)
{
    actionMapping.loadActions(inputActionsPath);

    JsonFile file(inputMappingPath);
    if (!file.isGood()) {
        throw std::runtime_error(
            fmt::format("failed to load input mapping from {}", inputMappingPath.string()));
    }
    const auto loader = file.getLoader();

    keyboard.loadMapping(loader.getLoader("keyboard"), actionMapping);
    gamepad.loadMapping(loader.getLoader("gamepad"), actionMapping);
}

void InputManager::onNewFrame()
{
    actionMapping.onNewFrame();
    keyboard.onNewFrame();
    mouse.onNewFrame();
    gamepad.onNewFrame();
}

void InputManager::handleEvent(const SDL_Event& event)
{
    switch (getEventCategory(event.type)) {
    case InputEventCategory::Keyboard:
        usingGamepad = false;
        keyboard.handleEvent(event, actionMapping);
        break;
    case InputEventCategory::Mouse:
        mouse.handleEvent(event, actionMapping);
        break;
    case InputEventCategory::Gamepad:
        usingGamepad = true;
        gamepad.handleEvent(event, actionMapping);
        break;
    default:
        break;
    }
}

void InputManager::update(float dt)
{
    keyboard.update(dt, actionMapping);
    gamepad.update(dt, actionMapping);

    actionMapping.update(dt);
}

void InputManager::cleanup()
{
    gamepad.cleanup();
}

void InputManager::resetInput()
{
    actionMapping.resetState();
    keyboard.resetInput();
    mouse.resetInput();
    gamepad.resetInput();
}

InputManager::InputEventCategory InputManager::getEventCategory(
    InputManager::SDLEventType type) const
{
    switch (type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        return InputEventCategory::Keyboard;
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEWHEEL:
    case SDL_MOUSEMOTION:
        return InputEventCategory::Mouse;
        break;
    case SDL_CONTROLLERAXISMOTION:
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
    case SDL_CONTROLLERDEVICEADDED:
    case SDL_CONTROLLERDEVICEREMOVED:
    case SDL_CONTROLLERDEVICEREMAPPED:
        return InputEventCategory::Gamepad;
        break;
    default:
        return InputEventCategory::None;
        break;
    }
}
