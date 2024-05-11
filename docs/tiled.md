# Tiled editor notes

* Set "Object alignment" property to top-left when using tilesets for placing objects. All objects inside Tiled map should be assumed to have top-left origin, no matter what origin scheme the game uses
* Flipping tile objects only works visually, be sure to add "Direction" custom property
* Object size is only used for "trigger" and "teleport" prefabs

## Custom properties

* "Direction" - "Left" or "Right"
* "name" - NPC's name - maps to `NPCComponent::name`
* "text" - default NPC's text - maps to `NPCComponent::defaultText`
* "level" - teleport destination level - maps to `TeleportComponent::levelTag`
* "to" - teleport destination level - maps to `TeleportComponent::levelTag`
