#include <edbr/GameCommon/CommonComponentLoaders.h>

#include <edbr/Core/JsonDataLoader.h>
#include <edbr/ECS/ComponentFactory.h>
#include <edbr/ECS/Components/MovementComponent.h>
#include <edbr/ECS/Components/NPCComponent.h>

namespace edbr
{
void registerMovementComponentLoader(ComponentFactory& cf)
{
    cf.registerComponentLoader(
        "movement", [](entt::handle e, MovementComponent& mc, const JsonDataLoader& loader) {
            loader.get("maxSpeed", mc.maxSpeed);
        });
}

void registerNPCComponentLoader(ComponentFactory& cf)
{
    cf.registerComponentLoader(
        "npc", [](entt::handle e, NPCComponent& npcc, const JsonDataLoader& loader) {
            std::string nameTag;
            loader.getIfExists("name", nameTag);
            if (!nameTag.empty()) {
                npcc.name = LST{nameTag};
            }

            std::string defaultTextTag;
            loader.getIfExists("text", defaultTextTag);
            if (!defaultTextTag.empty()) {
                npcc.defaultText = LST{defaultTextTag};
            }
        });
}

}
