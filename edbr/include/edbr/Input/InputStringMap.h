#pragma once

#include <string>

#include <SDL_gamecontroller.h>
#include <SDL_scancode.h>

SDL_Scancode toSDLScancode(const std::string& str);
const std::string& toString(SDL_Scancode key);

SDL_GameControllerButton toSDLGameControllerButton(const std::string& str);
const std::string& toString(SDL_GameControllerButton axis);

SDL_GameControllerAxis toSDLGameControllerAxis(const std::string& str);
const std::string& toString(SDL_GameControllerAxis axis);
