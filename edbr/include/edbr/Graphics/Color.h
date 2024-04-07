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

    static LinearColor FromRGB(
        std::uint8_t r,
        std::uint8_t g,
        std::uint8_t b,
        std::uint8_t a = 255);

    static LinearColor Black() { return {0.f, 0.f, 0.f, 1.f}; }
    static LinearColor White() { return {1.f, 1.f, 1.f, 1.f}; }
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

    // See http://www.brucelindbloom.com/index.html?Eqn_XYZ_to_RGB.html
    float gammaToLinear(float gamma)
    {
        return gamma < 0.04045f ? gamma / 12.92f :
                                  std::pow(maxF(gamma + 0.055f, 0.f) / 1.055f, 2.4f);
    }

    // See http://www.brucelindbloom.com/index.html?Eqn_XYZ_to_RGB.html
    float linearToGamma(float v)
    {
        return v < 0.00313108 ? 12.92 * v : 1.055 * std::pow(v, 1.f / 2.4f) - 0.055f;
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

inline RGBColor linearToRGB(const LinearColor& c)
{
    return RGBColor{
        (std::uint8_t)(linearToGamma(c.r) * 255.f),
        (std::uint8_t)(linearToGamma(c.g) * 255.f),
        (std::uint8_t)(linearToGamma(c.b) * 255.f),
        (std::uint8_t)(c.a * 255.f),
    };
}

} // end of namespace edbr
