#include <edbr/Camera/CameraActions.h>

#include <edbr/ActionList/ActionWrappers.h>
#include <edbr/Graphics/Camera.h>

#include <glm/gtx/compatibility.hpp> // lerp for vec3

namespace actions
{
std::unique_ptr<Action> moveCameraTo(Camera& camera, Transform targetTransform, float duration)
{
    const auto cameraStartPos = camera.getPosition();
    const auto cameraStartHeading = camera.getHeading();

    if (glm::dot(cameraStartHeading, targetTransform.getHeading()) < 0) {
        // this gives us the shortest rotation
        targetTransform.setHeading(-targetTransform.getHeading());
    }

    using namespace actions;
    return tween(
        {.startValue = 0.f,
         .endValue = 1.f,
         .duration = duration,
         .tween = glm::quadraticEaseInOut<float>,
         .setter = [&camera, cameraStartPos, cameraStartHeading, targetTransform](float t) {
             const auto currentPos = glm::lerp(cameraStartPos, targetTransform.getPosition(), t);
             camera.setPosition(currentPos);

             const auto currentHeading =
                 glm::slerp(cameraStartHeading, targetTransform.getHeading(), t);
             camera.setHeading(currentHeading);
         }});
}

std::unique_ptr<Action> moveCameraToTrack(
    Camera& camera,
    std::function<Transform()> transformGetter,
    float duration)
{
    const auto cameraStartPos = camera.getPosition();
    const auto cameraStartHeading = camera.getHeading();

    using namespace actions;
    return tween(
        {.startValue = 0.f,
         .endValue = 1.f,
         .duration = duration,
         .tween = glm::quadraticEaseInOut<float>,
         .setter = [&camera, cameraStartPos, cameraStartHeading, transformGetter](float t) {
             const auto targetTransform = transformGetter();
             const auto currentPos = glm::lerp(cameraStartPos, targetTransform.getPosition(), t);
             camera.setPosition(currentPos);

             const auto currentHeading =
                 glm::slerp(cameraStartHeading, targetTransform.getHeading(), t);
             camera.setHeading(currentHeading);
         }});
}
}
