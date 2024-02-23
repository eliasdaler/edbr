#pragma once

#include <span>

#include <glm/vec4.hpp>

#include <Math/Sphere.h>

namespace util
{
math::Sphere calculateBoundingSphere(std::span<glm::vec3> positions);
}
