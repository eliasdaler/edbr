#include "FreeCameraController.h"

#include <Graphics/Camera.h>
#include <util/InputUtil.h>

void FreeCameraController::handleInput(const Camera& camera)
{
    const auto camFront = camera.getTransform().getLocalFront();
    const auto camRight = camera.getTransform().getLocalRight();

    const auto moveStickState = util::getStickState({
        .up = SDL_SCANCODE_W,
        .down = SDL_SCANCODE_S,
        .left = SDL_SCANCODE_A,
        .right = SDL_SCANCODE_D,
    });
    glm::vec3 moveVector{};
    moveVector += camFront * (-moveStickState.y);
    moveVector += camRight * moveStickState.x;

    if (util::isKeyPressed(SDL_SCANCODE_Q)) {
        moveVector -= math::GlobalUpAxis / 2.f;
    }
    if (util::isKeyPressed(SDL_SCANCODE_E)) {
        moveVector += math::GlobalUpAxis / 2.f;
    }

    moveVelocity = moveVector * moveSpeed;
    if (util::isKeyPressed(SDL_SCANCODE_LSHIFT)) {
        moveVelocity *= 2.f;
    }

    const auto rotateStickState = util::getStickState({
        .up = SDL_SCANCODE_UP,
        .down = SDL_SCANCODE_DOWN,
        .left = SDL_SCANCODE_LEFT,
        .right = SDL_SCANCODE_RIGHT,
    });

    rotationVelocity.x = -rotateStickState.x * rotateSpeed;
    rotationVelocity.y = rotateStickState.y * rotateSpeed;
}

void FreeCameraController::update(Camera& camera, float dt)
{
    auto newPos = camera.getPosition();
    newPos += moveVelocity * dt;
    camera.setPosition(newPos);

    freeCameraYaw += rotationVelocity.x * dt;
    freeCameraPitch += rotationVelocity.y * dt;
    camera.setYawPitch(freeCameraYaw, freeCameraPitch);
}

void FreeCameraController::setYawPitch(float yaw, float pitch)
{
    freeCameraYaw = yaw;
    freeCameraPitch = pitch;
}
