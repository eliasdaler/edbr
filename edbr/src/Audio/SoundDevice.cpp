#include <edbr/Audio/SoundDevice.h>

#include <edbr/Audio/OpenALUtil.h>

#include <AL/alext.h>
#include <AL/efx-presets.h>
#include <AL/efx.h>

#include <iostream>

#include <cassert>
#include <cstring>

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

SoundDevice::SoundDevice()
{
    /* auto enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
    assert(enumeration);
    auto devices = enumerateOpenALDevices();
    for (const auto& d : devices) {
        std::cout << "OpenAL device: " << d << std::endl;
    } */

    device = alcOpenDevice(nullptr);
    if (!device) {
        std::cerr << "ERROR: Failed to create OpenAL device:" << std::endl;
        std::exit(1);
    }

    loadHRTF();

    if (!context) { // context was created when we loaded HRTF (Emscripten only)
        if (!alcCall(alcCreateContext, context, device, device, nullptr) || !context) {
            std::cerr << "ERROR: Could not create OpenAL audio context" << std::endl;
            std::exit(1);
        }
    }

    ALCboolean contextMadeCurrent = false;
    if (!alcCall(alcMakeContextCurrent, contextMadeCurrent, device, context) ||
        contextMadeCurrent != ALC_TRUE) {
        std::cerr << "ERROR: Could not make audio context current" << std::endl;
        std::exit(1);
    }

    assert(alGetString(AL_VERSION));
    printf("OpenAL version: %s\n", alGetString(AL_VERSION));
    printf("OpenAL vendor: %s\n", alGetString(AL_VENDOR));
    printf("OpenAL renderer: %s\n", alGetString(AL_RENDERER));

    ALCint srate;
    alcGetIntegerv(device, ALC_FREQUENCY, 1, &srate);
    fprintf(stdout, "Sample rate: %d\n", srate);

    // loadReverb();
}

SoundDevice::~SoundDevice()
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
        fprintf(stdout, "HRTF supported: 0\n");
        return;
    }

    fprintf(stdout, "HRTF supported: 1\n");

    int num_hrtf;
    alcGetIntegerv(device, ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &num_hrtf);
    if (!num_hrtf) {
        printf("No HRTFs found\n");
    } else {
        fprintf(stdout, "Num HRTFs found: %d\n", num_hrtf);
    }

    const char* hrtfname{nullptr};
    {
        ALCint attr[5];
        ALCint index = -1;
        ALCint i;

        printf("Available HRTFs:\n");
        for (i = 0; i < num_hrtf; i++) {
            const ALCchar* name = alcGetStringiSOFT(device, ALC_HRTF_SPECIFIER_SOFT, i);
            printf("    %d: %s\n", i, name);

            /* Check if this is the HRTF the user requested. */
            if (hrtfname && strcmp(name, hrtfname) == 0) index = i;
        }

        i = 0;
        attr[i++] = ALC_HRTF_SOFT;
        attr[i++] = ALC_TRUE;
        if (index == -1) {
            if (hrtfname) printf("HRTF \"%s\" not found\n", hrtfname);
            printf("Using default HRTF...\n");
        } else {
            printf("Selecting HRTF %d...\n", index);
            attr[i++] = ALC_HRTF_ID_SOFT;
            attr[i++] = index;
        }
        attr[i] = 0;

        if (!alcResetDeviceSOFT(device, attr))
            printf("Failed to reset device: %s\n", alcGetString(device, alcGetError(device)));
    }
}
