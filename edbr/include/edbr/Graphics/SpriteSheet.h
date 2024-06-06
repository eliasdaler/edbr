#pragma once

#include <vector>

#include <edbr/Math/Rect.h>

struct SpriteSheet {
    std::vector<math::IntRect> frames;

    math::IntRect getFrameRect(std::size_t frameNum) const
    {
        if (frameNum + 1 <= frames.size()) {
            return frames[frameNum];
        }
        return {};
    }
};
