#pragma once

#include <glm/vec2.hpp>

#include <SDL2/SDL_scancode.h>

namespace util
{
bool isKeyPressed(SDL_Scancode scancode);

struct StickBindings {
    SDL_Scancode up;
    SDL_Scancode down;
    SDL_Scancode left;
    SDL_Scancode right;
};

glm::vec2 getStickState(const StickBindings& bindings);
}
