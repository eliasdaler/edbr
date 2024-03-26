#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <edbr/Input/ActionTagHash.h>

class ActionMapping;
class Camera;

namespace util
{
glm::vec2 getStickState(
    const ActionMapping& actionMapping,
    const ActionTagHash& horizontalAxis,
    const ActionTagHash& verticalAxis);

// The function returns stick heading in world coordinates,
// stickState is in "screen" coordinates.
glm::vec3 calculateStickHeading(const Camera& camera, const glm::vec2& stickState);
}
