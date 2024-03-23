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

    const glm::mat4& asMatrix() const;

    Transform operator*(const Transform& rhs) const;
    Transform inverse() const;

    glm::vec3 getLocalUp() const { return heading * math::GlobalUpAxis; }
    glm::vec3 getLocalFront() const { return heading * math::GlobalFrontAxis; }
    glm::vec3 getLocalRight() const { return heading * math::GlobalRightAxis; }

    bool isIdentity() const;

    void setPosition(const glm::vec3& pos);
    void setHeading(const glm::quat& h);
    void setScale(const glm::vec3& s);

    const glm::vec3& getPosition() const { return position; }
    const glm::quat& getHeading() const { return heading; }
    const glm::vec3& getScale() const { return scale; }

private:
    glm::vec3 position{};
    glm::quat heading = glm::identity<glm::quat>();
    glm::vec3 scale{1.f};

    mutable glm::mat4 transformMatrix{1.f};
    mutable bool isDirty{false};
};
