#include <edbr/Util/InputUtil.h>

#include <edbr/Graphics/Camera.h>
#include <edbr/Input/ActionMapping.h>

#include <SDL2/SDL_keyboard.h>

namespace util
{
bool isKeyPressed(SDL_Scancode scancode)
{
    const Uint8* state = SDL_GetKeyboardState(nullptr);
    return state[scancode] != 0;
}

glm::vec2 getStickState(
    const ActionMapping& actionMapping,
    const ActionTagHash& horizontalAxis,
    const ActionTagHash& verticalAxis)
{
    return glm::
        vec2{actionMapping.getAxisValue(horizontalAxis), actionMapping.getAxisValue(verticalAxis)};
}

glm::vec3 calculateStickHeading(const Camera& camera, const glm::vec2& stickState)
{
    auto cameraFrontProj = camera.getTransform().getLocalFront();
    cameraFrontProj.y = 0.f;
    cameraFrontProj = glm::normalize(cameraFrontProj);

    const auto cameraRightProj = glm::normalize(glm::cross(cameraFrontProj, math::GlobalUpAxis));

    const auto rightMovement = cameraRightProj * stickState.x;
    const auto upMovement = cameraFrontProj * (-stickState.y);
    return glm::normalize(rightMovement + upMovement);
}

}
