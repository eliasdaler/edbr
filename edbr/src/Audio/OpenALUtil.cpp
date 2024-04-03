#include <edbr/Audio/OpenALUtil.h>

#include <AL/al.h>
#include <AL/alc.h>

#ifndef __EMSCRIPTEN__
#include <AL/efx.h>
#endif

#include <iostream>

bool check_al_errors(const char* filename, const std::uint32_t line)
{
    ALenum error = alGetError();
    if (error == AL_NO_ERROR) {
        return true;
    }

    std::cerr << "Error: (" << filename << ": " << line << ")\n";
    switch (error) {
    case AL_INVALID_NAME:
        std::cerr << "AL_INVALID_NAME: a bad name (ID) was passed to an OpenAL function";
        break;
    case AL_INVALID_ENUM:
        std::cerr << "AL_INVALID_ENUM: an invalid enum value was passed to an OpenAL function";
        break;
    case AL_INVALID_VALUE:
        std::cerr << "AL_INVALID_VALUE: an invalid value was passed to an OpenAL function";
        break;
    case AL_INVALID_OPERATION:
        std::cerr << "AL_INVALID_OPERATION: the requested operation is not valid";
        break;
    case AL_OUT_OF_MEMORY:
        std::cerr << "AL_OUT_OF_MEMORY: the requested operation resulted in OpenAL running out "
                     "of memory";
        break;
    default:
        std::cerr << "UNKNOWN AL ERROR: " << error;
    }
    std::cerr << std::endl;
    return false;
}

bool check_alc_errors(const char* filename, const std::uint_fast32_t line, ALCdevice* device)
{
    ALCenum error = alcGetError(device);
    if (error == ALC_NO_ERROR) {
        return true;
    }

    std::cerr << "***ERROR*** (" << filename << ": " << line << ")\n";
    switch (error) {
    case ALC_INVALID_VALUE:
        std::cerr << "ALC_INVALID_VALUE: an invalid value was passed to an OpenAL function";
        break;
    case ALC_INVALID_DEVICE:
        std::cerr << "ALC_INVALID_DEVICE: a bad device was passed to an OpenAL function";
        break;
    case ALC_INVALID_CONTEXT:
        std::cerr << "ALC_INVALID_CONTEXT: a bad context was passed to an OpenAL function";
        break;
    case ALC_INVALID_ENUM:
        std::cerr << "ALC_INVALID_ENUM: an unknown enum value was passed to an OpenAL function";
        break;
    case ALC_OUT_OF_MEMORY:
        std::cerr << "ALC_OUT_OF_MEMORY: an unknown enum value was passed to an OpenAL function";
        break;
    default:
        std::cerr << "UNKNOWN ALC ERROR: " << error;
    }
    std::cerr << std::endl;
    return false;
}

namespace util
{
void setOpenALListenerPosition(float x, float y, float z)
{
    alCall(alListener3f, AL_POSITION, x, y, z);
}

void setOpenALListenerOrientation(const std::array<float, 6>& orientation)
{
    alCall(alListenerfv, AL_ORIENTATION, orientation.data());
}

#ifndef __EMSCRIPTEN__
/* LoadEffect loads the given reverb properties into a new OpenAL effect
 * object, and returns the new effect ID. */
ALuint LoadEffect(const EFXEAXREVERBPROPERTIES* reverb)
{
    ALuint effect = 0;
    ALenum err;

    /* Create the effect object and check if we can do EAX reverb. */
    alGenEffects(1, &effect);
    if (alGetEnumValue("AL_EFFECT_EAXREVERB") != 0) {
        printf("Using EAX Reverb\n");

        /* EAX Reverb is available. Set the EAX effect type then load the
         * reverb properties. */
        alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

        alEffectf(effect, AL_EAXREVERB_DENSITY, reverb->flDensity);
        alEffectf(effect, AL_EAXREVERB_DIFFUSION, reverb->flDiffusion);
        alEffectf(effect, AL_EAXREVERB_GAIN, reverb->flGain);
        alEffectf(effect, AL_EAXREVERB_GAINHF, reverb->flGainHF);
        alEffectf(effect, AL_EAXREVERB_GAINLF, reverb->flGainLF);
        alEffectf(effect, AL_EAXREVERB_DECAY_TIME, reverb->flDecayTime);
        alEffectf(effect, AL_EAXREVERB_DECAY_HFRATIO, reverb->flDecayHFRatio);
        alEffectf(effect, AL_EAXREVERB_DECAY_LFRATIO, reverb->flDecayLFRatio);
        alEffectf(effect, AL_EAXREVERB_REFLECTIONS_GAIN, reverb->flReflectionsGain);
        alEffectf(effect, AL_EAXREVERB_REFLECTIONS_DELAY, reverb->flReflectionsDelay);
        alEffectfv(effect, AL_EAXREVERB_REFLECTIONS_PAN, reverb->flReflectionsPan);
        alEffectf(effect, AL_EAXREVERB_LATE_REVERB_GAIN, reverb->flLateReverbGain);
        alEffectf(effect, AL_EAXREVERB_LATE_REVERB_DELAY, reverb->flLateReverbDelay);
        alEffectfv(effect, AL_EAXREVERB_LATE_REVERB_PAN, reverb->flLateReverbPan);
        alEffectf(effect, AL_EAXREVERB_ECHO_TIME, reverb->flEchoTime);
        alEffectf(effect, AL_EAXREVERB_ECHO_DEPTH, reverb->flEchoDepth);
        alEffectf(effect, AL_EAXREVERB_MODULATION_TIME, reverb->flModulationTime);
        alEffectf(effect, AL_EAXREVERB_MODULATION_DEPTH, reverb->flModulationDepth);
        alEffectf(effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, reverb->flAirAbsorptionGainHF);
        alEffectf(effect, AL_EAXREVERB_HFREFERENCE, reverb->flHFReference);
        alEffectf(effect, AL_EAXREVERB_LFREFERENCE, reverb->flLFReference);
        alEffectf(effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, reverb->flRoomRolloffFactor);
        alEffecti(effect, AL_EAXREVERB_DECAY_HFLIMIT, reverb->iDecayHFLimit);
    } else {
        printf("Using Standard Reverb\n");

        /* No EAX Reverb. Set the standard reverb effect type then load the
         * available reverb properties. */
        alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);

        alEffectf(effect, AL_REVERB_DENSITY, reverb->flDensity);
        alEffectf(effect, AL_REVERB_DIFFUSION, reverb->flDiffusion);
        alEffectf(effect, AL_REVERB_GAIN, reverb->flGain);
        alEffectf(effect, AL_REVERB_GAINHF, reverb->flGainHF);
        alEffectf(effect, AL_REVERB_DECAY_TIME, reverb->flDecayTime);
        alEffectf(effect, AL_REVERB_DECAY_HFRATIO, reverb->flDecayHFRatio);
        alEffectf(effect, AL_REVERB_REFLECTIONS_GAIN, reverb->flReflectionsGain);
        alEffectf(effect, AL_REVERB_REFLECTIONS_DELAY, reverb->flReflectionsDelay);
        alEffectf(effect, AL_REVERB_LATE_REVERB_GAIN, reverb->flLateReverbGain);
        alEffectf(effect, AL_REVERB_LATE_REVERB_DELAY, reverb->flLateReverbDelay);
        alEffectf(effect, AL_REVERB_AIR_ABSORPTION_GAINHF, reverb->flAirAbsorptionGainHF);
        alEffectf(effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, reverb->flRoomRolloffFactor);
        alEffecti(effect, AL_REVERB_DECAY_HFLIMIT, reverb->iDecayHFLimit);
    }

    /* Check if an error occurred, and clean up if so. */
    err = alGetError();
    if (err != AL_NO_ERROR) {
        fprintf(stderr, "OpenAL error: %s\n", alGetString(err));
        if (alIsEffect(effect)) alDeleteEffects(1, &effect);
        return 0;
    }

    return effect;
}
#endif
}
