#pragma once

#include <span>

#include <glm/vec3.hpp>

#include <edbr/Math/Sphere.h>

namespace util
{
math::Sphere calculateBoundingSphere(std::span<glm::vec3> positions);
glm::vec3 smoothDamp(
    const glm::vec3& current,
    glm::vec3 target,
    glm::vec3& currentVelocity,
    float smoothTime,
    float dt,
    glm::vec3 maxSpeed = {
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
    });
}
