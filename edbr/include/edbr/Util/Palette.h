#pragma once

#include <edbr/Graphics/Color.h>
#include <vector>

namespace util
{

struct Palette {
    std::vector<RGBColor> colors;
};

extern Palette Arne16Palette;
extern Palette LightColorsPalette;

RGBColor pickRandomColor(const Palette& palette, void* ptr);
RGBColor pickRandomColor(const Palette& palette, std::uint64_t seed);

};
