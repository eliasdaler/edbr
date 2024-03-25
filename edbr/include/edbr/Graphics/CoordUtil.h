#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace edbr
{
namespace util
{
    inline glm::vec2 fromWindowCoordsToNDC(
        const glm::ivec2& windowCoords,
        const glm::ivec2& windowSize)
    {
        return static_cast<glm::vec2>(windowCoords) / static_cast<glm::vec2>(windowSize) * 2.f -
               glm::vec2{1.f};
    }

    // NDC = View * World => World = View^(-1) * NDC
    inline glm::vec3 fromNDCToWorldCoords(const glm::vec2& ndcCoords, const glm::mat4& viewProj)
    {
        const auto r = glm::inverse(viewProj) * glm::vec4{ndcCoords.x, ndcCoords.y, 0.f, 1.f};
        return glm::vec3(r);
    }

    // Converts window coordinate (where window size is windowSize) to a world coordinate
    // where the camera's transform is specified by cameraViewProj
    inline glm::vec3 fromScreenCoordsToWorldCoords(
        const glm::ivec2& screenCoords,
        const glm::ivec2& screenSize,
        const glm::mat4& cameraViewProj)
    {
        return fromNDCToWorldCoords(
            fromWindowCoordsToNDC(screenCoords, screenSize), cameraViewProj);
    }

    inline glm::vec2 pixelCoordToUV(const glm::ivec2& pixelCoord, const glm::ivec2& textureSize)
    {
        return static_cast<glm::vec2>(pixelCoord) / static_cast<glm::vec2>(textureSize);
    }

    inline static const glm::vec2 OFFSCREEN_COORD{-10000, -10000};

    inline glm::vec2 fromWorldCoordsToScreenCoords(
        const glm::vec3& worldPos,
        const glm::mat4& cameraViewProj,
        const glm::ivec2& screenSize)
    {
        const auto clip = cameraViewProj * glm::vec4{worldPos, 1.f};
        auto screen = glm::vec2(clip.x, clip.y) / clip.w;

        // hack: offscreen (but what else can we return here?)
        if (clip.w < 0.0f || screen.x >= 1.0f || screen.y >= 1.0f) {
            return OFFSCREEN_COORD;
        }

        screen = (screen + glm::vec2{1.f, 1.f}) / 2.f;
        screen *= screenSize;
        return screen;
    }

}
}
