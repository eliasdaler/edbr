#pragma once

#include <cstdint>

#include <glm/vec4.hpp>

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
} // end of namespace util
