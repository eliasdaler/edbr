#include <edbr/Audio/Music.h>

#include <edbr/Audio/OpenALUtil.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include <AL/al.h>
#include <vorbis/vorbisfile.h>

namespace
{
static constexpr std::size_t NUM_BUFFERS = 4;
static constexpr ALsizei BUFFER_SIZE = 65536;
}

namespace nuaudio
{

struct Music::StreamingAudioData {
    ALuint buffers[NUM_BUFFERS]{0}; // buffers that have next data ready
    std::filesystem::path filename; // name of the file being streamed
    std::ifstream file; // file stream
    std::uint8_t channels{0};
    std::int32_t sampleRate{0};
    std::uint8_t bitsPerSample{0};
    ALsizei size{0}; // total size of file in bytes
    ALuint source{0}; // source which sream will play from
    ALsizei sizeConsumed{0}; // current position in file
    ALenum format{0}; // format of stream
    OggVorbis_File oggVorbisFile; // handle to libvorbis file
    int oggCurrentSection{0}; // current section being played
    std::size_t duration{0}; // total length of audio data

    ALuint buffer{0};
};
}

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

void handle_ov_raw_seek_error(int seekResult)
{
    switch (seekResult) {
    case OV_ENOSEEK:
        std::cerr << "Error: OV_ENOSEEK found when trying to loop" << std::endl;
        break;
    case OV_EINVAL:
        std::cerr << "Error: OV_EINVAL found when trying to loop" << std::endl;
        break;
    case OV_EREAD:
        std::cerr << "Error: OV_EREAD found when trying to loop" << std::endl;
        break;
    case OV_EFAULT:
        std::cerr << "Error: OV_EFAULT found when trying to loop" << std::endl;
        break;
    case OV_EOF:
        std::cerr << "Error: OV_EOF found when trying to loop" << std::endl;
        break;
    case OV_EBADLINK:
        std::cerr << "Error: OV_EBADLINK found when trying to loop" << std::endl;
        break;
    default:
        break;
    }
}

void handle_ov_read_error(long result)
{
    switch (result) {
    case OV_HOLE:
        std::cerr << "Error: OV_HOLE found in update of buffer " << std::endl;
        break;
    case OV_EBADLINK:
        std::cerr << "Error: OV_EBADLINK found in update of buffer " << std::endl;
        break;
    case OV_EINVAL:
        std::cerr << "Error: OV_EINVAL found in update of buffer " << std::endl;
        break;
    default:
        break;
    }
}

// called by vorbis when it needs more data from file
std::size_t read_ogg_callback(
    void* destination,
    std::size_t size,
    std::size_t nmemb,
    void* fileHandle)
{
    auto& audioData = *reinterpret_cast<nuaudio::Music::StreamingAudioData*>(fileHandle);

    auto length = static_cast<ALsizei>(size * nmemb);

    if (audioData.sizeConsumed + length > audioData.size) {
        length = audioData.size - audioData.sizeConsumed;
    }

    // reopen the file if needed
    if (!audioData.file.is_open()) {
        audioData.file.open(audioData.filename.string(), std::ios::binary);
        if (!audioData.file.is_open()) {
            std::cerr << "Error: Could not re-open streaming file \"" << audioData.filename << "\""
                      << std::endl;
            return 0;
        }
    }

    std::vector<char> moreData;
    moreData.resize(length);

    audioData.file.clear();
    audioData.file.seekg(audioData.sizeConsumed);
    if (!audioData.file.read(&moreData[0], length)) {
        if (audioData.file.eof()) {
            audioData.file.clear(); // just clear the error, we will resolve it later
        } else if (audioData.file.fail()) {
            std::cerr << "Error: ogg stream has fail bit set " << audioData.filename << std::endl;
            audioData.file.clear();
            return 0;
        } else if (audioData.file.bad()) {
            std::cerr << "Error: ogg stream has bad bit set " << audioData.filename << std::endl;
            audioData.file.clear();
            return 0;
        }
    }
    audioData.file.clear();

    std::memcpy(destination, &moreData[0], length);
    audioData.sizeConsumed += length;
    return length;
}

int seek_ogg_callback(void* fileHandle, ogg_int64_t offset, int type)
{
    auto& audioData = *reinterpret_cast<nuaudio::Music::StreamingAudioData*>(fileHandle);

    switch (type) {
    case SEEK_CUR:
        audioData.sizeConsumed += static_cast<ALsizei>(offset);
        break;
    case SEEK_END:
        audioData.sizeConsumed = audioData.size - static_cast<ALsizei>(offset);
        break;
    case SEEK_SET:
        audioData.sizeConsumed = static_cast<ALsizei>(offset);
        break;
    default:
        return -1; // wrong seek type
    }

    if (audioData.sizeConsumed < 0) {
        audioData.sizeConsumed = 0;
        return -1;
    }

    if (audioData.sizeConsumed > audioData.size) {
        audioData.sizeConsumed = audioData.size;
        return -1;
    }

    return 0;
}

long tell_ogg_callback(void* fileHandle)
{
    auto& audioData = *reinterpret_cast<nuaudio::Music::StreamingAudioData*>(fileHandle);
    return audioData.sizeConsumed;
}

} // end of anonymous namespace

namespace nuaudio
{

Music::Music() : audioData(std::make_unique<Music::StreamingAudioData>())
{}

Music::~Music()
{
    if (audioData->source != 0) {
        if (audioData->buffer != 0) {
            alCall(alSourceUnqueueBuffers, audioData->source, 1, &audioData->buffer);
        }

        alCall(alDeleteSources, 1, &audioData->source);
        alCall(alDeleteBuffers, static_cast<ALsizei>(NUM_BUFFERS), &audioData->buffers[0]);

        ov_clear(&audioData->oggVorbisFile);
    }
}

bool Music::openFromFile(const std::filesystem::path& filename)
{
    auto& ad = *audioData;

    ad.filename = filename;
    ad.file.open(filename.string(), std::ios::binary);
    if (!ad.file.is_open()) {
        std::cerr << "Error: couldn't open file" << filename << std::endl;
        return false;
    }

    // calculate file size
    ad.file.seekg(0, std::ios_base::beg);
    ad.file.ignore(std::numeric_limits<std::streamsize>::max()); // read as much as possible
    ad.size = static_cast<ALsizei>(ad.file.gcount());
    ad.file.clear(); // clear eof flag

    // return to the beginning
    ad.file.seekg(0, std::ios_base::beg);
    ad.sizeConsumed = 0;

    // set callbacks
    ov_callbacks oggCallbacks;
    oggCallbacks.read_func = read_ogg_callback;
    oggCallbacks.close_func = nullptr; // we handle cleanup ourselves
    oggCallbacks.seek_func = seek_ogg_callback;
    oggCallbacks.tell_func = tell_ogg_callback;

    if (ov_open_callbacks(
            reinterpret_cast<void*>(audioData.get()),
            &ad.oggVorbisFile,
            nullptr,
            -1,
            oggCallbacks) < 0) {
        std::cerr << "Error: Could not ov_open_callbacks" << std::endl;
        return false;
    }

    // read file info
    vorbis_info* vorbisInfo = ov_info(&ad.oggVorbisFile, -1);
    ad.channels = vorbisInfo->channels;
    ad.bitsPerSample = 16;
    ad.sampleRate = vorbisInfo->rate;
    ad.duration = static_cast<std::size_t>(ov_time_total(&ad.oggVorbisFile, -1));

    // determine format
    const auto format = getSoundFormat(ad.channels, ad.bitsPerSample);
    if (format == INVALID_FORMAT) {
        std::cerr << "Error: unrecognised ogg format: " << ad.channels << " channels, "
                  << ad.bitsPerSample << " bps" << std::endl;
        return false;
    }
    ad.format = format;

    if (ad.source != 0) {
        alCall(alDeleteSources, 1, &ad.source);
    }
    // generate source
    alCall(alGenSources, 1, &ad.source);
    alCall(alSourcei, ad.source, AL_LOOPING, AL_FALSE);

    // generate buffers
    alCall(alGenBuffers, static_cast<ALsizei>(NUM_BUFFERS), &ad.buffers[0]);

    if (ad.file.eof()) {
        std::cerr << "Error: Already reached EOF without loading data" << std::endl;
        return false;
    } else if (ad.file.fail()) {
        std::cerr << "Error: Fail bit set" << std::endl;
        return false;
    } else if (!ad.file) {
        std::cerr << "Error: file is false" << std::endl;
        return false;
    }

    // init buffers initially
    std::vector<char> data;
    data.resize(BUFFER_SIZE);
    for (std::size_t i = 0; i < NUM_BUFFERS; ++i) {
        int dataSoFar = 0;
        while (dataSoFar < BUFFER_SIZE) {
            auto result = ov_read(
                &ad.oggVorbisFile,
                &data[dataSoFar],
                BUFFER_SIZE - dataSoFar,
                0,
                2,
                1,
                &ad.oggCurrentSection);

            if (result == OV_HOLE || result == OV_EBADLINK || result == OV_EINVAL) {
                handle_ov_read_error(result);
                break;
            }

            if (result == 0) {
                std::cerr << "Error: EOF found in initial read of buffer " << i << std::endl;
                break;
            }

            dataSoFar += result;
        }

        alCall(alBufferData, ad.buffers[i], ad.format, data.data(), dataSoFar, ad.sampleRate);
    }

    alCall(alSourceQueueBuffers, ad.source, static_cast<ALsizei>(NUM_BUFFERS), &ad.buffers[0]);

    return true;
}

void Music::play()
{
    if (getStatus() != Status::Paused) {
        alCall(alSourceStop, audioData->source);
    }
    alCall(alSourcePlay, audioData->source);
}

void Music::pause()
{
    alCall(alSourcePause, audioData->source);
}

void Music::update()
{
    ALint buffersProcessed = 0;
    alCall(alGetSourcei, audioData->source, AL_BUFFERS_PROCESSED, &buffersProcessed);
    if (buffersProcessed <= 0) {
        return;
    }

    while (buffersProcessed--) {
        ALuint b;
        alCall(alSourceUnqueueBuffers, audioData->source, 1, &b);
        audioData->buffer = b;

        std::vector<char> data;
        data.resize(BUFFER_SIZE);

        ALsizei dataSizeToBuffer = 0;
        int sizeRead = 0;

        while (sizeRead < BUFFER_SIZE) {
            const auto result = ov_read(
                &audioData->oggVorbisFile,
                &data[sizeRead],
                BUFFER_SIZE - sizeRead,
                0,
                2,
                1,
                &audioData->oggCurrentSection);
            if (result == OV_HOLE || result == OV_EBADLINK || result == OV_EINVAL) {
                handle_ov_read_error(result);
                break;
            }

            if (result == 0) { // end of data
                if (!isLooped()) {
                    // TODO: return to the beginning here properly
                    return;
                }

                // go to the beginning
                const auto seekResult = ov_raw_seek(&audioData->oggVorbisFile, 0);
                handle_ov_raw_seek_error(seekResult);
                if (seekResult != 0) { // other error
                    std::cerr << "Error: Unknown error in ov_raw_seek" << std::endl;
                    return;
                }
            }
            sizeRead += result;
        }
        dataSizeToBuffer = sizeRead;

        if (dataSizeToBuffer > 0) {
            alCall(
                alBufferData,
                audioData->buffer,
                audioData->format,
                data.data(),
                dataSizeToBuffer,
                audioData->sampleRate);
            alCall(alSourceQueueBuffers, audioData->source, 1, &audioData->buffer);
        }

        if (dataSizeToBuffer < BUFFER_SIZE) {
            std::cerr << "Data is missing" << std::endl;
        }

        // Check state and restart if needed
        if (getStatus() != Status::Playing && isLooped()) {
            play();
        }
    }
}

void Music::stop()
{
    alCall(alSourceStop, audioData->source);
}

void Music::setLooped(bool b)
{
    // note that we don't set source to be looping, because it'll loop just one buffer!
    looped = b;
}

bool Music::isLooped() const
{
    return looped;
}

Music::Status Music::getStatus() const
{
    ALint status;
    alCall(alGetSourcei, audioData->source, AL_SOURCE_STATE, &status);

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

void Music::setVolume(float volume)
{
    alCall(alSourcef, audioData->source, AL_GAIN, volume * 0.01f);
}
}
