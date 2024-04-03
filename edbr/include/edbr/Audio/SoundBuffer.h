#pragma once

#include <filesystem>

#include <AL/al.h>

namespace nuaudio
{
class SoundBuffer {
public:
    SoundBuffer();
    SoundBuffer(SoundBuffer&& o) noexcept;
    ~SoundBuffer();
    SoundBuffer& operator=(SoundBuffer&& o) noexcept;

    bool loadFromFile(const std::filesystem::path& p);

    ALuint getBuffer() const { return buffer; }

private:
    ALuint buffer{0};
    std::filesystem::path path;
};
}
