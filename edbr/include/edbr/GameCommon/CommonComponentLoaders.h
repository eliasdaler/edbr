#pragma once

class ComponentFactory;

namespace edbr
{
void registerMovementComponentLoader(ComponentFactory& cf);
void registerNPCComponentLoader(ComponentFactory& cf);
}
