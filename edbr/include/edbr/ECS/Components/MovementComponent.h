#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

struct MovementComponent {
    glm::vec3 kinematicVelocity; // manual velocity for kinematic objects
    glm::vec3 maxSpeed; // only for kinematic speed

    glm::vec3 prevFramePosition;
    glm::vec3 effectiveVelocity; // (newPos - prevPos) / dt

    // smooth rotation
    glm::quat startHeading;
    glm::quat targetHeading;
    float rotationTime{0.f};
    float rotationProgress{0.f}; // from [0 to rotationTime] (used for slerp)
};
