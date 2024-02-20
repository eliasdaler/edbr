#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <Math/Transform.h>

class Camera {
public:
    enum class OriginType {
        TopLeft, // (0,0) is at the top left corner of the screen
        Center, // (0,0) is the center of the screen
    };

public:
    // fovX is in radians
    void init(float fovX, float zNear, float zFar, float aspectRatio);

    // 3d orthographic camera scale is how many units the camera sees
    // (e.g. if scale == 1, then camera sees from -1 to 1)
    void initOrtho(float scale, float zNear, float zFar);
    void initOrtho(float xScale, float yScale, float zNear, float zFar);

    // +Y axis will point down, +X - right, +Z - into the camera
    void initOrtho2D(
        const glm::vec2& size,
        float zNear,
        float zFar,
        OriginType origin = OriginType::TopLeft);

    glm::mat4 getView() const;
    glm::mat4 getViewProj() const;

    void setYawPitch(float yaw, float pitch);

    const Transform& getTransform() const { return transform; }

    void setPosition(const glm::vec3& pos) { transform.position = pos; }
    const glm::vec3 getPosition() const { return transform.position; }

    void setHeading(const glm::quat& h) { transform.heading = h; }
    const glm::quat& getHeading() const { return transform.heading; }

    void setProjection(const glm::mat4& proj) { projection = proj; }
    const glm::mat4& getProjection() const { return projection; }

    bool isOrthographic() const { return orthographic; }

    float getZFar() const { return zFar; };
    float getZNear() const { return zNear; };
    float getAspectRatio() const { return aspectRatio; }
    float getFOVX() const { return fovX; }
    float getFOVY() const { return fovY; }

private:
    Transform transform;

    glm::mat4 projection;
    bool orthographic{false};

    float zNear{1.f};
    float zFar{75.f};
    float aspectRatio{16.f / 9.f};
    float fovX{glm::radians(90.f)}; // horizontal fov in radians
    float fovY{glm::radians(60.f)}; // vertical fov in radians

    // only for ortographic cameras
    glm::vec2 viewSize; // the area which the camera covers
};
