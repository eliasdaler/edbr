#pragma once

#include <array>

#include <glm/fwd.hpp>

class Camera;

Camera calculateCSMCamera(
    const std::array<glm::vec3, 8>& frustumCorners,
    const glm::vec3& lightDir,
    float shadowMapSize);
