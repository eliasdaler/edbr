#pragma once

#include <filesystem>

struct ImageData {
    ImageData() = default;
    ~ImageData();

    // move only
    ImageData(ImageData&& o) = default;
    ImageData& operator=(ImageData&& o) = default;

    // no copies
    ImageData(const ImageData& o) = delete;
    ImageData& operator=(const ImageData& o) = delete;

    // data
    unsigned char* pixels{nullptr};
    int width{0};
    int height{0};
    int channels{0};

    // HDR only
    float* hdrPixels{nullptr};
    bool hdr{false};
    int comp{0};

    bool shouldSTBFree{false};
};

namespace util
{
ImageData loadImage(const std::filesystem::path& p);
}
