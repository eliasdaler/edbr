#include <edbr/Core/JsonGraphics.h>

#include <nlohmann/json.hpp>

void from_json(const nlohmann::json& j, LinearColor& c)
{
    if (j.is_array()) {
        if (j.size() == 3) {
            c = LinearColor{j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), 1.f};
        } else {
            assert(j.size() == 4);
            c = LinearColor{
                j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()};
        }
    } else {
        // special case for RGB colors e.g. "color": { "rgb": [255, 80, 10] }
        const auto& rgb = j.at("rgb");
        if (rgb.size() == 3) {
            const auto rgbColor = RGBColor{
                rgb[0].get<std::uint8_t>(),
                rgb[1].get<std::uint8_t>(),
                rgb[2].get<std::uint8_t>(),
                255};
            c = edbr::rgbToLinear(rgbColor);
        } else {
            assert(rgb.size() == 4);
            const auto rgbColor = RGBColor{
                rgb[0].get<std::uint8_t>(),
                rgb[1].get<std::uint8_t>(),
                rgb[2].get<std::uint8_t>(),
                rgb[3].get<std::uint8_t>()};
            c = edbr::rgbToLinear(rgbColor);
        }
    }
}
