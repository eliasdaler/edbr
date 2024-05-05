#pragma once

#include <glm/vec2.hpp>

struct SpriteAnimation {
    // Note that frame numbering starts from 1, so "0" is an "invalid" frame number
    int startFrame{0};
    int endFrame{0};

    float frameDuration; // seconds
    glm::vec2 origin;
    bool looped{true};

    int getFrameCount() const { return endFrame - startFrame + 1; }
    float getDuration() const { return frameDuration * getFrameCount(); }
};
