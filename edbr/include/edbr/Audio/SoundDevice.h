#pragma once

#include <string>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>

class SoundDevice {
public:
    bool init();
    void cleanup();

    ALCdevice* getDevice() const { return device; }
    ALCcontext* getContext() const { return context; }

    bool hasReverb() const { return reverbActivated; }
    ALuint getEffectSlot() const { return slot; }

private:
    void loadHRTF();
    void loadReverb();

    ALCdevice* device{nullptr};
    ALCcontext* context{nullptr};

    ALuint slot{0};
    ALuint effect{0};
    bool reverbActivated{false};
};
