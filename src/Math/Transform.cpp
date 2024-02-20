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

glm::mat4 Transform::asMatrix() const
{
    // TODO: cache + dirty flag for optimization?
    auto tm = glm::translate(I, position);
    tm *= glm::mat4_cast(heading);
    tm = glm::scale(tm, scale);
    return tm;
}
