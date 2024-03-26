#include <edbr/Graphics/Letterbox.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace util
{
glm::vec4 calculateLetterbox(const glm::ivec2& drawImageSize, const glm::ivec2& swapchainSize)
{
    const auto ir = glm::vec2{drawImageSize.x, drawImageSize.y};
    const auto scale = std::min(swapchainSize.x / ir.x, swapchainSize.y / ir.y);
    const auto ss = glm::vec2{swapchainSize} / (ir * scale);

    std::array<float, 4> vp{0.f, 0.f, 1.f, 1.f};

    // letterboxing
    if (ss.x > ss.y) { // won't fit horizontally - add vertical bars
        vp[2] = ss.y / ss.x;
        vp[0] = (1.f - vp[2]) / 2.f; // center horizontally
    } else { // won'f fit vertically - add horizonal bars (pillarboxing)
        vp[3] = ss.x / ss.y;
        vp[1] = (1.f - vp[3]) / 2.f; // center vertically
    }

    const auto destX = std::ceil(vp[0] * swapchainSize.x);
    const auto destY = std::ceil(vp[1] * swapchainSize.y);
    const auto destW = std::ceil(vp[2] * swapchainSize.x);
    const auto destH = std::ceil(vp[3] * swapchainSize.y);

    return {(int)destX, (int)destY, (int)destW, (int)destH};
}
}
