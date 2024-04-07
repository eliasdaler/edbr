#include <edbr/Graphics/Color.h>

LinearColor LinearColor::FromRGB(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
{
    return edbr::rgbToLinear(r, g, b, a);
}
