#pragma once

class EntityInfoDisplayer;

namespace edbr
{
void registerTagComponentDisplayer(EntityInfoDisplayer& eid);
void registerMovementComponentDisplayer(EntityInfoDisplayer& eid);
void registerNPCComponentDisplayer(EntityInfoDisplayer& eid);
}
