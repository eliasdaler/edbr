#include <edbr/Util/CameraUtil.h>

#include <edbr/Graphics/Camera.h>
#include <edbr/Math/Transform.h>

namespace util
{
glm::vec3 getOrbitCameraTarget(
    const Transform& entityTransform,
    float cameraYOffset,
    float cameraZOffset)
{
    return entityTransform.getPosition() + entityTransform.getLocalUp() * cameraYOffset +
           entityTransform.getLocalFront() * cameraZOffset;
}

glm::vec3 getOrbitCameraDesiredPosition(
    const Camera& camera,
    const Transform& entityTransform,
    float cameraYOffset,
    float cameraZOffset,
    float orbitDistance)
{
    const auto trackPoint = getOrbitCameraTarget(entityTransform, cameraYOffset, cameraZOffset);
    return getOrbitCameraDesiredPosition(camera, trackPoint, orbitDistance);
}

glm::vec3 getOrbitCameraDesiredPosition(
    const Camera& camera,
    const glm::vec3& trackPoint,
    float orbitDistance)
{
    return trackPoint - camera.getTransform().getLocalFront() * orbitDistance;
}

} // end of namespace util
