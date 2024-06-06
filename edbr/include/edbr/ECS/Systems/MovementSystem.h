#pragma once

#include <entt/fwd.hpp>

namespace edbr::ecs
{
void movementSystemUpdate(entt::registry& registry, float dt);
void movementSystemPostPhysicsUpdate(entt::registry& registry, float dt);
}
