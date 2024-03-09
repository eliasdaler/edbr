#pragma once

#include <glm/vec4.hpp>

// Representation of light data on GPU (see lighting.glsl)
struct GPULightData {
    // [ vec3(position), type ]
    glm::vec4 positionAndType;
    // [ vec3(direction), range ]
    glm::vec4 directionAndRange;
    // [ color.r, color.g, color.b, intesity]
    glm::vec4 colorAndIntensity;
    // [ scaleOffset.x, scaleOffset.y, shadowMapIndex, <unused> ]
    glm::vec4 scaleOffsetAndSMIndexAndUnused;
};
