#pragma once

#include <span>

#include <glm/vec3.hpp>

#include <edbr/Math/Sphere.h>

namespace util
{
math::Sphere calculateBoundingSphere(std::span<glm::vec3> positions);
}
