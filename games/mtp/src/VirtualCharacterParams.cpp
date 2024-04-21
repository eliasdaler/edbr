#include "VirtualCharacterParams.h"

#include <edbr/Core/JsonDataLoader.h>

void VirtualCharacterParams::loadFromJson(const JsonDataLoader& loader)
{
    float maxSlopeAngleDeg{};
    loader.get("maxSlopeAngleDeg", maxSlopeAngleDeg, 60.f);
    maxSlopeAngle = glm::radians(maxSlopeAngleDeg);

    loader.get("maxStrength", maxStrength, 100.f);
    loader.get("characterPadding", characterPadding, 0.02f);
    loader.get("penetrationRecoverySpeed", penetrationRecoverySpeed, 1.f);
    loader.get("predictiveContactDistance", predictiveContactDistance, 0.1f);
    loader.get("characterRadius", characterRadius);
    loader.get("characterHeight", characterHeight);
    loader.get("enableWalkStairs", enableWalkStairs, false);
    loader.get("enableStickToFloor", enableStickToFloor, true);
    loader.get("enableCharacterInertia", enableCharacterInertia, false);
    loader.get("runSpeed", runSpeed);
    loader.get("walkSpeed", walkSpeed);
    loader.get("jumpSpeed", jumpSpeed, 0.f);
    loader.get("controlMovementDuringJump", controlMovementDuringJump, true);
    loader.get("gravityFactor", gravityFactor, 1.f);
    loader.get("smallJumpFactor", smallJumpFactor, 4.f);
}
