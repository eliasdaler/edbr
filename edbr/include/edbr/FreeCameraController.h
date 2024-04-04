#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class Camera;
class InputManager;

class FreeCameraController {
public:
    void handleInput(const InputManager& im, const Camera& camera);
    void update(Camera& camera, float dt);

    float rotateYawSpeed{1.75f};
    float rotatePitchSpeed{1.5f};

private:
    float freeCameraYaw{0.f};
    float freeCameraPitch{0.f};

    glm::vec3 moveVelocity{};
    glm::vec3 moveSpeed{10.f, 5.f, 10.f};

    glm::vec2 rotationVelocity{}; // x - yaw, y - pitch
};