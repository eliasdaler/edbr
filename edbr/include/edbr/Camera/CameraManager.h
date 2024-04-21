#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <edbr/Camera/CameraController.h>
#include <edbr/Math/Transform.h>

class Camera;
class InputManager;
class ActionListManager;

class CameraManager {
public:
    CameraManager(ActionListManager& actionListManager);

    void handleInput(const InputManager& im, const Camera& camera, float dt);
    void update(Camera& camera, float dt);

    void addController(std::string name, std::unique_ptr<CameraController> controller);

    void addFreeCameraController(std::unique_ptr<CameraController> controller);
    void addFollowCameraController(std::unique_ptr<CameraController> controller);

    void setController(const std::string& tag);
    void setController(const std::string& tag, Camera& camera, float transitionTime = 0.f);
    CameraController& getController(const std::string& tag);
    const std::string& getCurrentControllerTag() { return currentControllerTag; }

    std::string staticCameraTag{"static"};

    void setCamera(const Transform& transform, Camera& camera, float transitionTime = 0.f);

    void stopCameraTransitions();

private:
    ActionListManager& actionListManager;
    std::string cameraTransitionActionListTag{"camera_transition"};

    std::unordered_map<std::string, std::unique_ptr<CameraController>> controllers;

    CameraController* currentController{nullptr};
    std::string currentControllerTag;
};
