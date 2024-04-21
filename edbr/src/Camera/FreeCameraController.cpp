#include <edbr/Camera/FreeCameraController.h>

#include <edbr/Graphics/Camera.h>
#include <edbr/Input/InputManager.h>
#include <edbr/Util/InputUtil.h>

#include <fmt/printf.h>

void FreeCameraController::handleInput(const InputManager& im, const Camera& camera, float dt)
{
    const auto& actionMapping = im.getActionMapping();
    static const auto moveXAxis = actionMapping.getActionTagHash("CameraMoveX");
    static const auto moveYAxis = actionMapping.getActionTagHash("CameraMoveY");
    static const auto moveZAxis = actionMapping.getActionTagHash("CameraMoveZ");
    static const auto moveFastAction = actionMapping.getActionTagHash("CameraMoveFast");

    const auto camFront = camera.getTransform().getLocalFront();
    const auto camRight = camera.getTransform().getLocalRight();

    const auto moveStickState = util::getStickState(actionMapping, moveXAxis, moveYAxis);
    const auto zAxisState = actionMapping.getAxisValue(moveZAxis);

    glm::vec3 moveVector{};
    moveVector += camFront * (-moveStickState.y);
    moveVector += camRight * moveStickState.x;
    moveVector += math::GlobalUpAxis * zAxisState;

    moveVelocity = moveVector * moveSpeed;

    if (actionMapping.isHeld(moveFastAction)) {
        moveVelocity *= 2.f;
    }

    static const auto rotateXAxis = actionMapping.getActionTagHash("CameraRotateX");
    static const auto rotateYAxis = actionMapping.getActionTagHash("CameraRotateY");
    const auto rotateStickState = util::getStickState(actionMapping, rotateXAxis, rotateYAxis);

    rotationVelocity.x = -rotateStickState.x * rotateYawSpeed;
    rotationVelocity.y = -rotateStickState.y * rotatePitchSpeed;
}

void FreeCameraController::update(Camera& camera, float dt)
{
    auto newPos = camera.getPosition();
    newPos += moveVelocity * dt;
    camera.setPosition(newPos);

    const auto deltaYaw = rotationVelocity.x * dt;
    const auto deltaPitch = rotationVelocity.y * dt;
    const auto dYaw = glm::angleAxis(deltaYaw, math::GlobalUpAxis);
    const auto dPitch = glm::angleAxis(deltaPitch, math::GlobalRightAxis);
    camera.setHeading(dYaw * camera.getHeading() * dPitch);
}
