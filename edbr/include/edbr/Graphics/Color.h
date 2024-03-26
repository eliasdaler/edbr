#pragma once

#include <cmath>
#include <cstdint>

// Non-linear color (RGBA8)
struct RGBColor {
    std::uint8_t r{0};
    std::uint8_t g{0};
    std::uint8_t b{0};
    std::uint8_t a{255};
};

struct LinearColor {
    float r{0.f};
    float g{0.f};
    float b{0.f};
    float a{1.f};
};

struct LinearColorNoAlpha {
    LinearColorNoAlpha() = default;
    LinearColorNoAlpha(const LinearColor& c) : r(c.r), g(c.g), b(c.b) {}

    float r{0.f};
    float g{0.f};
    float b{0.f};
};

namespace edbr
{
namespace
{
    float maxF(float a, float b)
    {
        return a < b ? b : a;
    }

    float gammaToLinear(float gamma)
    {
        return gamma < 0.04045 ? gamma / 12.92 : std::pow(maxF(gamma + 0.055, 0.0) / 1.055, 2.4);
    }
} // end of anonymous namespace

inline LinearColor rgbToLinear(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 1)
{
    return LinearColor{
        gammaToLinear((float)r / 255.f),
        gammaToLinear((float)g / 255.f),
        gammaToLinear((float)b / 255.f),
        (float)a / 255.f,
    };
}

inline LinearColor rgbToLinear(const RGBColor& color)
{
    return rgbToLinear(color.r, color.g, color.b, color.a);
}

} // end of namespace edbr
