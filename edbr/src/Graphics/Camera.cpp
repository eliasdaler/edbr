#include <edbr/Graphics/Camera.h>

#include <edbr/Math/GlobalAxes.h>

void Camera::init(float fovX, float zNear, float zFar, float aspectRatio)
{
    // see 6.1 in Foundations of Game Engine Development by Eric Lengyel
    float s = aspectRatio;
    float g = s / glm::tan(fovX / 2.f);
    fovY = 2.f * glm::atan(1.f / g);

    this->fovX = fovX;
    this->zFar = zFar;
    this->zNear = zNear;
    this->aspectRatio = aspectRatio;

    if (useInverseDepth) {
        projection = glm::perspective(fovY, aspectRatio, zFar, zNear);
    } else {
        projection = glm::perspective(fovY, aspectRatio, zNear, zFar);
    }

    if (clipSpaceYDown) {
        projection[1][1] *= -1;
    }

    initialized = true;
}

void Camera::initOrtho(float scale, float zNear, float zFar)
{
    initOrtho(scale, scale, zNear, zFar);
}

void Camera::initOrtho(float xScale, float yScale, float zNear, float zFar)
{
    orthographic = true;
    this->zFar = zFar;
    this->zNear = zNear;
    this->xScale = xScale;
    this->yScale = yScale;
    aspectRatio = xScale / yScale;

    if (useInverseDepth) {
        projection = glm::ortho(-xScale, xScale, -yScale, yScale, zFar, zNear);
    } else {
        projection = glm::ortho(-xScale, xScale, -yScale, yScale, zNear, zFar);
    }

    if (clipSpaceYDown) {
        projection[1][1] *= -1;
    }

    initialized = true;
}

void Camera::initOrtho2D(const glm::vec2& size, float zNear, float zFar, OriginType origin)
{
    setClipSpaceYDown(true);
    orthographic2D = true;
    this->zFar = zFar;
    this->zNear = zNear;
    aspectRatio = size.x / size.y;

    switch (origin) {
    case OriginType::TopLeft:
        projection = glm::ortho(0.f, size.x, 0.f, size.y, zNear, zFar);
        break;
    case OriginType::Center:
        projection =
            glm::ortho(-size.x / 2.f, size.x / 2.f, size.y / 2.f, -size.y / 2.f, zNear, zFar);
        break;
    default:
        assert(false);
    }

    viewSize = size;
    initialized = true;
}

glm::mat4 Camera::getView() const
{
    assert(initialized);
    if (orthographic2D) {
        return glm::translate(glm::mat4{1.f}, -transform.getPosition());
    }
    const auto up = transform.getLocalUp();
    const auto target = getPosition() + transform.getLocalFront();
    return glm::lookAt(getPosition(), target, up);
}

glm::mat4 Camera::getViewProj() const
{
    assert(initialized);
    return projection * getView();
}

void Camera::setYawPitch(float yaw, float pitch)
{
    const auto yawRotation = glm::angleAxis(yaw, math::GlobalUpAxis);
    const auto pitchRotation = glm::angleAxis(pitch, math::GlobalRightAxis);
    setHeading(yawRotation * pitchRotation);
}

void Camera::setUseInverseDepth(bool b)
{
    useInverseDepth = b;
    reinit();
}

void Camera::setClipSpaceYDown(bool b)
{
    clipSpaceYDown = b;
    reinit();
}

void Camera::reinit()
{
    if (orthographic) {
        initOrtho(xScale, yScale, zNear, zFar);
    } else {
        init(fovX, zNear, zFar, aspectRatio);
    }
}
