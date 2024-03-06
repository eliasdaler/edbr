#include "Transform.h"

#include <glm/gtx/matrix_decompose.hpp>

namespace
{
static const auto I = glm::mat4{1.f};
}

Transform::Transform(const glm::mat4& m)
{
    glm::vec3 scaleVec;
    glm::quat rotationQuat;
    glm::vec3 translationVec;
    glm::vec3 skewVec;
    glm::vec4 perspectiveVec;
    glm::decompose(m, scaleVec, rotationQuat, translationVec, skewVec, perspectiveVec);

    position = translationVec;
    heading = rotationQuat;
    scale = scaleVec;
    isDirty = true;
}

Transform Transform::operator*(const Transform& rhs) const
{
    if (isIdentity()) {
        return rhs;
    }
    if (rhs.isIdentity()) {
        return *this;
    }
    return Transform(asMatrix() * rhs.asMatrix());
}

Transform Transform::inverse() const
{
    if (isIdentity()) {
        return Transform{};
    }
    return Transform(glm::inverse(asMatrix()));
}

const glm::mat4& Transform::asMatrix() const
{
    if (!isDirty) {
        return transformMatrix;
    }

    transformMatrix = glm::translate(I, position);
    transformMatrix *= glm::mat4_cast(heading);
    transformMatrix = glm::scale(transformMatrix, scale);
    isDirty = false;
    return transformMatrix;
}

void Transform::setPosition(const glm::vec3& pos)
{
    position = pos;
    isDirty = true;
}

void Transform::setHeading(const glm::quat& h)
{
    heading = h;
    isDirty = true;
}

void Transform::setScale(const glm::vec3& s)
{
    scale = s;
    isDirty = true;
}

bool Transform::isIdentity() const
{
    return asMatrix() == glm::mat4{1.f};
}
