#pragma once

#include <glm/fwd.hpp>

#include <entt/entity/fwd.hpp>

namespace entityutil
{
// transform
void setWorldPosition2D(entt::handle e, const glm::vec2& pos);
glm::vec2 getWorldPosition2D(entt::const_handle e);
void setHeading2D(entt::handle e, const glm::vec2& u);
glm::vec2 getHeading2D(entt::const_handle e);

// persistency
void makePersistent(entt::handle e);
void makeNonPersistent(entt::handle e);

// animation
void setSpriteAnimation(entt::handle e, const std::string& animName);

// player
entt::handle getPlayerEntity(entt::registry& registry);
} // end of namespace entityutil
