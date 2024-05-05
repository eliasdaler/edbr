#pragma once

#include <vector>

#include <edbr/Math/Rect.h>

struct SpriteSheet {
    struct Frame {
        int frameNum;
        math::IntRect frameRect;
    };

    std::vector<Frame> frames;

    math::IntRect getFrameRect(int fn) const
    {
        for (const auto& f : frames) {
            if (f.frameNum == fn) {
                return f.frameRect;
            }
        }
        return {};
    }
};
