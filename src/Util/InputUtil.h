#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <SDL2/SDL_scancode.h>

class Camera;

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

// The function returns stick heading in world coordinates,
// stickState is in "screen" coordinates.
glm::vec3 calculateStickHeading(const Camera& camera, const glm::vec2& stickState);
}
