#include "Camera.h"

#include <Math/GlobalAxes.h>

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

    projection = glm::perspective(fovY, aspectRatio, zNear, zFar);
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
    aspectRatio = xScale / yScale;

    projection = glm::ortho(-xScale, xScale, -yScale, yScale, zNear, zFar);
}

void Camera::initOrtho2D(const glm::vec2& size, float zNear, float zFar, OriginType origin)
{
    orthographic = true;
    this->zFar = zFar;
    this->zNear = zNear;
    aspectRatio = size.x / size.y;

    switch (origin) {
    case OriginType::TopLeft:
        projection = glm::ortho(0.f, size.x, size.y, 0.f, zNear, zFar);
        break;
    case OriginType::Center:
        projection =
            glm::ortho(-size.x / 2.f, size.x / 2.f, size.y / 2.f, -size.y / 2.f, zNear, zFar);
        break;
    default:
        assert(false);
    }
    viewSize = size;
    // assert(getHeading() == glm::identity<glm::quat>() && "can't set heading for 2D camera");

    // 180 degree rotation around Y so that -Z is "forward" and +X is "right"
    setHeading({0, 0, 1, 0});
}

glm::mat4 Camera::getView() const
{
    const auto up = transform.getLocalUp();
    const auto target = getPosition() + transform.getLocalFront();
    return glm::lookAt(getPosition(), target, up);
}

glm::mat4 Camera::getViewProj() const
{
    return projection * getView();
}

void Camera::setYawPitch(float yaw, float pitch)
{
    const auto hd = glm::angleAxis(yaw, math::GlobalUpAxis);
    const auto hd2 = glm::angleAxis(pitch, math::GlobalLeftAxis);
    setHeading(hd * hd2);
}
