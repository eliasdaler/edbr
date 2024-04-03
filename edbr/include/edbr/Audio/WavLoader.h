#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <AL/al.h>

#include <filesystem>

namespace nuaudio
{
struct WavFile {
    std::vector<char> data;
    std::uint8_t channels{0};
    std::int32_t sampleRate{0};
    std::uint8_t bitsPerSample{0};
    ALsizei size{0};
};

bool loadWav(const std::filesystem::path& filename, WavFile& file);
}
