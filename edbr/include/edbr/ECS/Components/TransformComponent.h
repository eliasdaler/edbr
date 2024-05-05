#pragma once

#include <edbr/Math/Transform.h>

struct TransformComponent {
    Transform transform; // local (relative to parent)
    glm::mat4 worldTransform{1.f};
};
