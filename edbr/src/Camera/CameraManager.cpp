#include <edbr/Camera/CameraManager.h>

#include <edbr/ActionList/ActionListManager.h>
#include <edbr/ActionList/ActionWrappers.h>
#include <edbr/Graphics/Camera.h>

#include <edbr/Camera/CameraActions.h>

#include <cassert>

CameraManager::CameraManager(ActionListManager& actionListManager) :
    actionListManager(actionListManager)
{
    addController(staticCameraTag, std::make_unique<CameraController>());
    setController(staticCameraTag);
}

void CameraManager::handleInput(const InputManager& im, const Camera& camera, float dt)
{
    assert(currentController);
    currentController->handleInput(im, camera, dt);
}

void CameraManager::update(Camera& camera, float dt)
{
    assert(currentController);
    currentController->update(camera, dt);
}

void CameraManager::addController(std::string name, std::unique_ptr<CameraController> controller)
{
    auto [it, inserted] = controllers.emplace(std::move(name), std::move(controller));
    assert(inserted);
}

void CameraManager::setController(const std::string& tag)
{
    currentController = controllers.at(tag).get();
    currentControllerTag = tag;
}

void CameraManager::setController(const std::string& tag, Camera& camera, float transitionTime)
{
    setController(tag);
    if (transitionTime != 0.f) {
        stopCameraTransitions();
        actionListManager.addActionList(ActionList(
            cameraTransitionActionListTag,
            actions::moveCameraToTrack(
                camera,
                [this]() { return currentController->getDesiredTransform(); },
                transitionTime)));
    }
}

CameraController& CameraManager::getController(const std::string& tag)
{
    return *controllers.at(tag);
}

void CameraManager::setCamera(const Transform& transform, Camera& camera, float transitionTime)
{
    if (transitionTime == 0.f) {
        camera.setPosition(transform.getPosition());
        camera.setHeading(transform.getHeading());
    } else {
        stopCameraTransitions();
        actionListManager.addActionList(ActionList(
            cameraTransitionActionListTag,
            actions::moveCameraTo(camera, transform, transitionTime)));
    }
    setController(staticCameraTag);
}

void CameraManager::stopCameraTransitions()
{
    if (actionListManager.isActionListPlaying(cameraTransitionActionListTag)) {
        actionListManager.stopActionList(cameraTransitionActionListTag);
    }
}
