#pragma once

#include <vector>

#include <glm/vec4.hpp>

#include <Math/Sphere.h>

namespace util
{
math::Sphere calculateBoundingSphere(const std::vector<glm::vec4>& positions);
}
