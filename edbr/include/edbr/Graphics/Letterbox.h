#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace util
{
// Returns {x, y, w, h} or draw image after letter/pillar boxing will be applied
glm::vec4 calculateLetterbox(const glm::ivec2& drawImageSize, const glm::ivec2& swapchainSize);
}
