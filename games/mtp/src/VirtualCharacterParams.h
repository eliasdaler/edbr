#pragma once

#include <glm/trigonometric.hpp>

class JsonDataLoader;

struct VirtualCharacterParams {
    void loadFromJson(const JsonDataLoader& loader);

    // data
    float maxSlopeAngle = glm::radians(60.0f);
    float maxStrength{100.0f}; // controls how heavy objects the player can push
    float characterPadding{0.02f}; // difference between virtual shape and real shape
    float penetrationRecoverySpeed{1.0f};
    float predictiveContactDistance{0.1f};

    float characterRadius{0.3f}; // radius of the capsule, required
    float characterHeight{0.8f}; // height of the capsule, required

    bool enableWalkStairs{false}; // if true - can walk over small obstacles
    bool enableStickToFloor{true}; // if true, will stick to floor on slopes
    bool enableCharacterInertia{false}; // if true, will accumulate walk/run speed slowly

    float runSpeed{4.f}; // required
    float walkSpeed{1.15f}; // required
    float jumpSpeed{8.f}; // if not set - character won't be able to jump
    bool controlMovementDuringJump{true};
    float gravityFactor{1.5f};
    float smallJumpFactor{4.f}; // controls "small hop" jump (when the jump button is quickly
                                // pressed and not held)
};
