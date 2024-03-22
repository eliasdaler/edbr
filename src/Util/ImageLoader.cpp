#include "ImageLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

ImageData::~ImageData()
{
    if (shouldSTBFree) {
        stbi_image_free(pixels);
        stbi_image_free(hdrPixels);
    }
}

namespace util
{
ImageData loadImage(const std::filesystem::path& p)
{
    ImageData data;
    data.shouldSTBFree = true;
    if (stbi_is_hdr(p.string().c_str())) {
        data.hdr = true;
        data.hdrPixels = stbi_loadf(p.string().c_str(), &data.width, &data.height, &data.comp, 4);
    } else {
        data.pixels = stbi_load(p.string().c_str(), &data.width, &data.height, &data.channels, 4);
    }
    data.channels = 4;
    return data;
}

} // namespace util
