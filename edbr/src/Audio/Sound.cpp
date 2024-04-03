#include <edbr/Audio/Sound.h>

#include <edbr/Audio/OpenALUtil.h>
#include <edbr/Audio/SoundBuffer.h>

#include <AL/efx.h>

#include <cassert>

namespace nuaudio
{
Sound::Sound() : buffer(nullptr)
{
    alCall(alGenSources, 1, &source);
}

Sound::Sound(const SoundBuffer& buffer) : buffer(nullptr)
{
    alCall(alGenSources, 1, &source);
    setBuffer(buffer);
}

Sound::Sound(const Sound& o) : buffer(nullptr)
{
    alCall(alGenSources, 1, &source);

    if (o.buffer) {
        setBuffer(*o.buffer);
    }
}

Sound::Sound(Sound&& o) : buffer(o.buffer), source(o.source)
{
    o.source = 0;
    o.buffer = nullptr;
}

Sound::~Sound()
{
    if (source != 0) {
        if (buffer) {
            alCall(alSourcei, source, AL_BUFFER, 0);
        }
        alCall(alDeleteSources, 1, &source);
    }
}

Sound& Sound::operator=(const Sound& o)
{
    if (o.buffer) {
        setBuffer(*o.buffer);
    } else {
        buffer = nullptr;
    }
    return *this;
}

Sound& Sound::operator=(Sound&& o)
{
    buffer = o.buffer;
    source = o.source;
    o.buffer = nullptr;
    o.source = 0;
    return *this;
}

void Sound::setBuffer(const SoundBuffer& b)
{
    buffer = &b;
    alCall(alSourcei, source, AL_BUFFER, buffer->getBuffer());
}

void Sound::setLooped(bool b)
{
    alCall(alSourcei, source, AL_LOOPING, b ? AL_TRUE : AL_FALSE);
}

bool Sound::isLooped() const
{
    ALint loop;
    alCall(alGetSourcei, source, AL_LOOPING, &loop);
    return loop != 0;
}

void Sound::play()
{
    alCall(alSourcePlay, source);
}

void Sound::pause()
{
    alCall(alSourcePause, source);
}

void Sound::stop()
{
    alCall(alSourceStop, source);
}

void Sound::setPosition(float x, float y, float z)
{
    alCall(alSource3f, source, AL_POSITION, x, y, z);
}

void Sound::setReferenceDistance(float dist)
{
    alCall(alSourcef, source, AL_REFERENCE_DISTANCE, dist);
}

void Sound::setMaxDistance(float dist)
{
    alCall(alSourcef, source, AL_MAX_DISTANCE, dist);
}

void Sound::setPitch(float pitch)
{
    alCall(alSourcef, source, AL_PITCH, pitch);
}

void Sound::setRolloffFactor(float f)
{
    alCall(alSourcef, source, AL_ROLLOFF_FACTOR, f);
}

Sound::Status Sound::getStatus() const
{
    ALint status;
    alCall(alGetSourcei, source, AL_SOURCE_STATE, &status);

    switch (status) {
    case AL_INITIAL:
    case AL_STOPPED:
        return Status::Stopped;
    case AL_PAUSED:
        return Status::Paused;
    case AL_PLAYING:
        return Status::Playing;
    }

    return Status::Stopped;
}

void Sound::setEffectSlot(ALuint slot)
{
    alSource3i(source, AL_AUXILIARY_SEND_FILTER, (ALint)slot, 0, AL_FILTER_NULL);
    assert(alGetError() == AL_NO_ERROR && "Failed to setup sound source");
}
}
