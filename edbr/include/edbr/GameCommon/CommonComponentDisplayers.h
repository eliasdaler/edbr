#pragma once

class EntityInfoDisplayer;

namespace edbr
{
void registerMetaInfoComponentDisplayer(EntityInfoDisplayer& eid);
void registerTagComponentDisplayer(EntityInfoDisplayer& eid);
void registerMovementComponentDisplayer(EntityInfoDisplayer& eid);
void registerNPCComponentDisplayer(EntityInfoDisplayer& eid);
}
