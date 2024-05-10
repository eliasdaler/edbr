#pragma once

#include <string>

#include <entt/entity/fwd.hpp>

namespace entityutil
{
// tag
entt::handle getEntityByTag(entt::registry& registry, const std::string& tag);
void setTag(entt::handle e, const std::string& tag);
const std::string& getTag(entt::const_handle e);

// persistency
void makePersistent(entt::handle e);
void makeNonPersistent(entt::handle e);

// kinematic movement
void stopKinematicMovement(entt::handle e);
void stopKinematicRotation(entt::handle e);
}
