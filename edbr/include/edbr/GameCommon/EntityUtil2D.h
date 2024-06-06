#pragma once

#include <string>

#include <entt/entity/fwd.hpp>
#include <glm/fwd.hpp>

#include <edbr/Math/Rect.h>

namespace entityutil
{
// transform
void setWorldPosition2D(entt::handle e, const glm::vec2& pos);
glm::vec2 getWorldPosition2D(entt::const_handle e);
void setHeading2D(entt::handle e, const glm::vec2& u);
glm::vec2 getHeading2D(entt::const_handle e);

// sprite graphics
math::FloatRect getSpriteWorldRect(entt::const_handle e);
void setSpriteAnimation(entt::handle e, const std::string& animName);

// collision
math::FloatRect getCollisionAABB(entt::const_handle e);
}
