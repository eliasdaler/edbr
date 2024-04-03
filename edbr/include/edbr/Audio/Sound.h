#pragma once

#include <AL/al.h>

namespace nuaudio
{
class SoundBuffer;

class Sound {
public:
    enum class Status { Stopped, Playing, Paused };

    Sound();
    Sound(const SoundBuffer& buffer);
    Sound(const Sound& o);
    Sound(Sound&& o);
    ~Sound();

    Sound& operator=(const Sound& o);
    Sound& operator=(Sound&& o);

    void setBuffer(const SoundBuffer& b);

    void play();
    void pause();
    void stop();

    void setPosition(float x, float y, float z);

    // distance under with the volume would drop in half
    void setReferenceDistance(float dist);

    // used with the Inverse Clamped Distance Model to set the distance where
    // there will no longer be any attenuation of the source
    void setMaxDistance(float dist);

    void setRolloffFactor(float f);
    void setPitch(float pitch);

    void setLooped(bool b);
    bool isLooped() const;

    Status getStatus() const;

    void setEffectSlot(ALuint i);

private:
    const SoundBuffer* buffer{nullptr};
    ALuint source;
};
}
