#pragma once

#include <glm/fwd.hpp>

#include <entt/entity/fwd.hpp>

#include <edbr/Math/Rect.h>

namespace entityutil
{
entt::handle getEntityByTag(entt::registry& registry, const std::string& tag);
void setTag(entt::handle e, const std::string& tag);

// transform
void setWorldPosition2D(entt::handle e, const glm::vec2& pos);
glm::vec2 getWorldPosition2D(entt::const_handle e);
void setHeading2D(entt::handle e, const glm::vec2& u);
glm::vec2 getHeading2D(entt::const_handle e);

// collision
math::FloatRect getAABB(entt::const_handle e);

// persistency
void makePersistent(entt::handle e);
void makeNonPersistent(entt::handle e);

// graphics
math::FloatRect getSpriteWorldRect(entt::const_handle e);

// animation
void setSpriteAnimation(entt::handle e, const std::string& animName);

// player
entt::handle getPlayerEntity(entt::registry& registry);
void spawnPlayer(entt::registry& registry, const std::string& spawnName);

// interaction
entt::handle findInteractableEntity(entt::registry& registry);

} // end of namespace entityutil
