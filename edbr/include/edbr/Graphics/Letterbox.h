#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace util
{
// Returns {x, y, w, h} of draw image after letter/pillar boxing will be applied
// (image of inputSize will be fit inside image of outputSize)
glm::vec4 calculateLetterbox(
    const glm::ivec2& inputSize,
    const glm::ivec2& outputSize,
    bool integerScale = false);
}
