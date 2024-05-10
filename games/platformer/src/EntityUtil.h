#pragma once

#include <glm/fwd.hpp>

#include <entt/entity/fwd.hpp>

#include <edbr/Math/Rect.h>

#include <edbr/GameCommon/EntityUtil.h>
#include <edbr/GameCommon/EntityUtil2D.h>

namespace entityutil
{
// collision
math::FloatRect getCollisionAABB(entt::const_handle e);

// player
entt::handle getPlayerEntity(entt::registry& registry);
void spawnPlayer(entt::registry& registry, const std::string& spawnName);

// interaction
entt::handle findInteractableEntity(entt::registry& registry);

} // end of namespace entityutil
