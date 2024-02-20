#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "GlobalAxes.h"

class Transform {
public:
    Transform() = default;
    Transform(const glm::mat4& m);

    bool operator==(const Transform& rhs) const
    {
        return position == rhs.position && heading == rhs.heading && scale == rhs.scale;
    }

    glm::mat4 asMatrix() const;

    Transform operator*(const Transform& rhs) const;
    Transform inverse() const;

    glm::vec3 getLocalUp() const { return heading * math::GlobalUpAxis; }
    glm::vec3 getLocalFront() const { return heading * math::GlobalFrontAxis; }
    glm::vec3 getLocalRight() const { return heading * math::GlobalRightAxis; }

    bool isIdentity() const
    {
        return position == glm::vec3{} && heading == glm::identity<glm::quat>() &&
               scale == glm::vec3{1.f};
    }

    glm::vec3 position{};
    glm::quat heading = glm::identity<glm::quat>();
    glm::vec3 scale{1.f};
};
