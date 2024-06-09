#include <edbr/Audio/SoundDevice.h>

#include <edbr/Audio/OpenALUtil.h>

#include <AL/alext.h>
#include <AL/efx-presets.h>
#include <AL/efx.h>

#include <cassert>
#include <cstring>

#include <fmt/printf.h>

// #define DEBUG_AUDIO_DEVICE

#ifdef DEBUG_AUDIO_DEVICE
#define AUDIO_DEBUG_PRINT(...) fmt::println(__VA_ARGS__);
#else
#define AUDIO_DEBUG_PRINT(...)
#endif

namespace
{
std::vector<std::string> enumerateOpenALDevices()
{
    std::vector<std::string> devicesList;
    const ALCchar* deviceNames;

    if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT")) {
        deviceNames = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
    } else {
        deviceNames = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
    }

    while (deviceNames && *deviceNames) {
        devicesList.emplace_back(deviceNames);
        deviceNames += strlen(deviceNames) + 1;
    }
    return devicesList;
}

}

bool SoundDevice::init()
{
    std::vector<std::string> devices;

    const auto enumExtPresent = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
    if (enumExtPresent) {
        devices = enumerateOpenALDevices();
        for (const auto& d : devices) {
            AUDIO_DEBUG_PRINT("Found OpenAL device: {}", d);
        }
    } else {
        AUDIO_DEBUG_PRINT("ALC_ENUMERATION_EXT is not present");
    }

    // try to open default device
    const std::string defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);
    AUDIO_DEBUG_PRINT("Default OpenAL device: {}", defaultDeviceName);
    device = alcOpenDevice(defaultDeviceName.c_str());

    if (!device) {
        // failed to open default device, try others...
        for (const auto& d : devices) {
            if (d == defaultDeviceName) {
                continue;
            }

            AUDIO_DEBUG_PRINT("Trying to open OpenAL device: {}", d);
            device = alcOpenDevice(d.c_str());
            if (device) {
                break;
            }
        }
    }

    if (!device) {
        fmt::println("[error] Failed to create OpenAL device");
        return false;
    }

    loadHRTF();

    if (!alcCall(alcCreateContext, context, device, device, nullptr)) {
        fmt::println("[error] Failed to create OpenAL audio context");
        return false;
    }

    ALCboolean contextMadeCurrent = false;
    if (!alcCall(alcMakeContextCurrent, contextMadeCurrent, device, context) ||
        contextMadeCurrent != ALC_TRUE) {
        fmt::println("[error] Failed to make OpenAL audio context current");
        return false;
    }

    { // print debug info
        assert(alGetString(AL_VERSION));
        AUDIO_DEBUG_PRINT("OpenAL version: {}", alGetString(AL_VERSION));
        AUDIO_DEBUG_PRINT("OpenAL vendor: {}", alGetString(AL_VENDOR));
        AUDIO_DEBUG_PRINT("OpenAL renderer: {}", alGetString(AL_RENDERER));

        ALCint srate;
        alcGetIntegerv(device, ALC_FREQUENCY, 1, &srate);
        AUDIO_DEBUG_PRINT("Sample rate: {}", srate);
    }

    // loadReverb();

    return true;
}

void SoundDevice::cleanup()
{
    ALCboolean contextMadeCurrent = false;
    if (!alcCall(alcMakeContextCurrent, contextMadeCurrent, device, nullptr)) {
        // do nothing
    }

    if (hasReverb()) {
        if (!alcCall(alDeleteAuxiliaryEffectSlots, device, 1, &slot)) {
            // do nothing
        }

        if (!alcCall(alDeleteEffects, device, 1, &effect)) {
            // do nothing
        }
    }

    if (!alcCall(alcDestroyContext, device, context)) {
        // do nothing
    }

    ALCboolean closed;
    if (!alcCall(alcCloseDevice, closed, device, device)) {
        // do nothing
    }
}

void SoundDevice::loadReverb()
{
    if (!alcIsExtensionPresent(alcGetContextsDevice(alcGetCurrentContext()), "ALC_EXT_EFX")) {
        fprintf(stderr, "Error: EFX not supported\n");
        return;
    }

    /* Load the reverb into an effect. */
    EFXEAXREVERBPROPERTIES reverb = EFX_REVERB_PRESET_SPORT_GYMNASIUM;
    effect = util::LoadEffect(&reverb);
    if (!effect) {
        std::exit(1);
    }

    if (!alcCall(alGenAuxiliaryEffectSlots, device, 1, &slot)) {
        std::exit(1);
    }

    /* Tell the effect slot to use the loaded effect object. Note that the this
     * effectively copies the effect properties. You can modify or delete the
     * effect object afterward without affecting the effect slot.
     */
    if (!alcCall(alAuxiliaryEffectSloti, device, slot, AL_EFFECTSLOT_EFFECT, (ALint)effect)) {
        std::exit(1);
    }

    reverbActivated = true;
}

void SoundDevice::loadHRTF()
{
    if (!alcIsExtensionPresent(device, "ALC_SOFT_HRTF")) {
        AUDIO_DEBUG_PRINT("HRTF supported: false");
        return;
    }

    AUDIO_DEBUG_PRINT("HRTF supported: true");

    int num_hrtf;
    alcGetIntegerv(device, ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &num_hrtf);
    if (!num_hrtf) {
        AUDIO_DEBUG_PRINT("No HRTFs found");
    } else {
        AUDIO_DEBUG_PRINT("Num HRTFs found: {}", num_hrtf);
    }

    const char* hrtfname{nullptr};
    {
        ALCint attr[5];
        ALCint index = -1;
        ALCint i;

        AUDIO_DEBUG_PRINT("Available HRTFs:");
        for (i = 0; i < num_hrtf; i++) {
            const ALCchar* name = alcGetStringiSOFT(device, ALC_HRTF_SPECIFIER_SOFT, i);
            AUDIO_DEBUG_PRINT("    {}: {}", i, name);

            /* Check if this is the HRTF the user requested. */
            if (hrtfname && strcmp(name, hrtfname) == 0) index = i;
        }

        i = 0;
        attr[i++] = ALC_HRTF_SOFT;
        attr[i++] = ALC_TRUE;
        if (index == -1) {
            if (hrtfname) {
                AUDIO_DEBUG_PRINT("HRTF {} not found", hrtfname);
            }
            AUDIO_DEBUG_PRINT("Using default HRTF...");
        } else {
            AUDIO_DEBUG_PRINT("Selecting HRTF {}...", index);
            attr[i++] = ALC_HRTF_ID_SOFT;
            attr[i++] = index;
        }
        attr[i] = 0;

        if (!alcResetDeviceSOFT(device, attr))
            fmt::printf(
                "[error] failed to reset device: %s\n", alcGetString(device, alcGetError(device)));
    }
}
