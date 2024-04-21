#pragma once

#include <glm/vec3.hpp>

class Transform;
class Camera;

namespace util
{
glm::vec3 getOrbitCameraTarget(
    const Transform& entityTransform,
    float cameraYOffset,
    float cameraZOffset);

glm::vec3 getOrbitCameraDesiredPosition(
    const Camera& camera,
    const Transform& entityTransform,
    float cameraYOffset,
    float cameraZOffset,
    float orbitDistance);

glm::vec3 getOrbitCameraDesiredPosition(
    const Camera& camera,
    const glm::vec3& trackPoint,
    float orbitDistance);
} // end of namespace util
