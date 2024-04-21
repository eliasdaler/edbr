#pragma once

#include <functional>
#include <memory>

class Camera;
class Transform;
class Action;

namespace actions
{
std::unique_ptr<Action> moveCameraTo(Camera& camera, Transform targetTransform, float duration);
std::unique_ptr<Action> moveCameraToTrack(
    Camera& camera,
    std::function<Transform()> transformGetter,
    float duration);
}
