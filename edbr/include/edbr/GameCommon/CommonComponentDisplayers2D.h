#pragma once

class EntityInfoDisplayer;

namespace edbr
{
void registerTransformComponentDisplayer2D(EntityInfoDisplayer& eid);
void registerCollisionComponent2DDisplayer(EntityInfoDisplayer& eid);
void registerSpriteAnimationComponentDisplayer(EntityInfoDisplayer& eid);
}
