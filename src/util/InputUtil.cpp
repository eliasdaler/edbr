#include "InputUtil.h"

#include <SDL2/SDL_keyboard.h>

namespace util
{
bool isKeyPressed(SDL_Scancode scancode)
{
    const Uint8* state = SDL_GetKeyboardState(nullptr);
    return state[scancode] != 0;
}

glm::vec2 getStickState(const StickBindings& bindings)
{
    glm::vec2 stick{};
    if (isKeyPressed(bindings.up)) {
        stick.y -= 1.f;
    }
    if (isKeyPressed(bindings.down)) {
        stick.y += 1.f;
    }
    if (isKeyPressed(bindings.left)) {
        stick.x -= 1.f;
    }
    if (isKeyPressed(bindings.right)) {
        stick.x += 1.f;
    }
    return stick;
}

}
