#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <glm/vec4.hpp>

namespace
{
float gammaToLinear(float gamma)
{
    return gamma < 0.04045 ? gamma / 12.92 : std::pow(std::max(gamma + 0.055, 0.0) / 1.055, 2.4);
}
}

namespace util
{
inline glm::vec4 colorRGB(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    return glm::vec4{
        (float)r / 255.f,
        (float)g / 255.f,
        (float)b / 255.f,
        1.f,
    };
}

inline glm::vec4 sRGBToLinear(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    return glm::vec4{
        gammaToLinear((float)r / 255.f),
        gammaToLinear((float)g / 255.f),
        gammaToLinear((float)b / 255.f),
        1.f,
    };
}
} // end of namespace util
