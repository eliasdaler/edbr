#pragma once

#include <string>
#include <unordered_map>

class SpriteAnimationData;

class ComponentFactory;

namespace edbr
{
void registerSpriteComponentLoader(ComponentFactory& cf);
void registerCollisionComponent2DLoader(ComponentFactory& cf);
void registerSpriteAnimationComponentLoader(
    ComponentFactory& cf,
    const std::unordered_map<std::string, SpriteAnimationData>& animationsData);
}
