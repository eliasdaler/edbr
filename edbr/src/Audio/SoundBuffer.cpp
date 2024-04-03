#include <edbr/Audio/SoundBuffer.h>

#include <edbr/Audio/OpenALUtil.h>
#include <edbr/Audio/WavLoader.h>

#include <iostream>

namespace
{
static const ALenum INVALID_FORMAT = 0;

ALenum getSoundFormat(std::uint8_t channels, std::uint8_t bitsPerSample)
{
    ALenum format;
    if (channels == 1 && bitsPerSample == 8)
        format = AL_FORMAT_MONO8;
    else if (channels == 1 && bitsPerSample == 16)
        format = AL_FORMAT_MONO16;
    else if (channels == 2 && bitsPerSample == 8)
        format = AL_FORMAT_STEREO8;
    else if (channels == 2 && bitsPerSample == 16)
        format = AL_FORMAT_STEREO16;
    else {
        return INVALID_FORMAT;
    }
    return format;
}
}

namespace nuaudio
{
SoundBuffer::SoundBuffer() : buffer(0)
{
    alCall(alGenBuffers, 1, &buffer);
}

SoundBuffer::SoundBuffer(SoundBuffer&& o) noexcept : buffer(o.buffer)
{
    o.buffer = 0;
}

SoundBuffer::~SoundBuffer()
{
    if (buffer != 0) {
        // TODO: sometimes fails, investigate
        alCall(alDeleteBuffers, 1, &buffer);
    }
}

SoundBuffer& SoundBuffer::operator=(SoundBuffer&& o) noexcept
{
    buffer = o.buffer;
    o.buffer = 0;
    return *this;
}

bool SoundBuffer::loadFromFile(const std::filesystem::path& p)
{
    path = p;
    WavFile file;
    if (!loadWav(p, file)) {
        std::cerr << "Failed to open " << p << std::endl;
        return false;
    }

    if (buffer != 0) {
        alCall(alDeleteBuffers, 1, &buffer);
    }

    if (!alCall(alGenBuffers, 1, &buffer)) {
        return false;
    }

    ALenum format = getSoundFormat(file.channels, file.bitsPerSample);
    if (format == INVALID_FORMAT) {
        std::cerr << "ERROR: unrecognised wave format: " << file.channels << " channels, "
                  << file.bitsPerSample << " bps" << std::endl;
        return false;
    }

    if (!alCall(
            alBufferData,
            buffer,
            format,
            file.data.data(),
            static_cast<ALsizei>(file.data.size()),
            file.sampleRate)) {
        return false;
    }

    return true;
}
}
