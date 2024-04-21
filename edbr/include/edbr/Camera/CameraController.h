#pragma once

#include <edbr/Math/Transform.h>

class Camera;
class InputManager;

class CameraController {
public:
    virtual ~CameraController() = default;

    virtual void handleInput(const InputManager& im, const Camera& camera, float dt){};
    virtual void update(Camera& camera, float dt){};

    virtual Transform getDesiredTransform() { return {}; }
};
