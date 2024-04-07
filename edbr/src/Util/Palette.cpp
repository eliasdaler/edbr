#include <edbr/Util/Palette.h>

#include <random>

namespace util
{
Palette Arne16Palette = Palette{
    .colors = {
        RGBColor(0, 0, 0), // black
        RGBColor(157, 157, 157), // gray
        RGBColor(255, 255, 255), // white
        RGBColor(190, 38, 51), // red
        RGBColor(224, 111, 139), // pink
        RGBColor(73, 60, 43), // dark brown
        RGBColor(164, 100, 34), // brown
        RGBColor(235, 137, 49), // orange
        RGBColor(247, 226, 107), // yellow
        RGBColor(47, 72, 78), // dark green
        RGBColor(68, 137, 26), // green
        RGBColor(163, 206, 39), // light green
        RGBColor(27, 38, 50), // very dark blue
        RGBColor(0, 87, 132), // dark blue
        RGBColor(49, 162, 242), // blue
        RGBColor(178, 220, 239), // light blue
    }};

Palette LightColorsPalette = Palette{
    .colors = {
        RGBColor(255, 255, 255), // white
        RGBColor(224, 111, 139), // pink
        RGBColor(235, 137, 49), // orange
        RGBColor(247, 226, 107), // yellow
        RGBColor(68, 137, 26), // green
        RGBColor(163, 206, 39), // light green
        RGBColor(49, 162, 242), // blue
        RGBColor(178, 220, 239), // light blue
        RGBColor(162, 255, 203), // light cyanish green,
        RGBColor(203, 243, 130), // light green,
        RGBColor(255, 219, 162), // light orange
        RGBColor(255, 186, 235), // light violet
        RGBColor(195, 178, 255), // light blueish violet

    }};

RGBColor pickRandomColor(const Palette& palette, void* ptr)
{
    return pickRandomColor(palette, (std::uint64_t) reinterpret_cast<std::intptr_t>(ptr));
}

RGBColor pickRandomColor(const Palette& palette, std::uint64_t seed)
{
    std::default_random_engine randEngine(seed);
    std::uniform_int_distribution<std::size_t> scaleDist(0, palette.colors.size() - 1);
    return palette.colors[scaleDist(randEngine)];
}

} // end of namespace util
