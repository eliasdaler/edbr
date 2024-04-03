#include <edbr/Audio/WavLoader.h>

#include <bit>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>

#include <AL/al.h>

namespace
{

std::int32_t convertToInt(char* buffer, std::size_t len)
{
    std::int32_t a = 0;
    if (std::endian::native == std::endian::little) {
        std::memcpy(&a, buffer, len);
    } else {
        for (std::size_t i = 0; i < len; ++i) {
            reinterpret_cast<char*>(&a)[3 - i] = buffer[i];
        }
    }
    return a;
}

template<typename T>
bool readInt(std::ifstream& file, char* buffer, const char* fieldName, int numBytes, T& res)
{
    if (!file.read(buffer, numBytes)) {
        std::cerr << "Error: could not read " << fieldName << std::endl;
        return false;
    }
    res = static_cast<T>(convertToInt(buffer, numBytes));
    return true;
}

bool readInt(std::ifstream& file, char* buffer, const char* fieldName, int numBytes)
{
    int res{};
    return readInt(file, buffer, fieldName, numBytes, res);
}

bool checkField(
    std::ifstream& file,
    char* buffer,
    const char* fieldName,
    const char* expected,
    int numBytes)
{
    if (!file.read(buffer, numBytes)) {
        std::cerr << "Error: could not read " << fieldName << std::endl;
        return false;
    }

    if (expected && (std::strncmp(buffer, expected, numBytes) != 0)) {
        std::cerr << "Error: " << fieldName << " doesn't contain '" << expected << "', got '"
                  << std::string(buffer, numBytes) << "' instead" << std::endl;
    }
    return true;
}

bool checkField(std::ifstream& file, char* buffer, const char* fieldName, int numBytes)
{
    return checkField(file, buffer, fieldName, nullptr, numBytes);
}

bool loadWavFileHeader(std::ifstream& file, nuaudio::WavFile& f)
{
    char buffer[4];
    if (!file.is_open()) {
        return false;
    }

    struct checkParams {
        const char* fieldName{nullptr};
        const char* expected{nullptr};
        int numBytes{0};
    };

    // intro
    for (const auto& p : {
             checkParams{"RIFF", "RIFF", 4},
             checkParams{"size of file", nullptr, 4},
             checkParams{"WAVE", "WAVE", 4},
             checkParams{"fmt/0", nullptr, 4},
             checkParams{"header size", nullptr, 4},
             checkParams{"PCM", nullptr, 2},
         }) {
        if (!checkField(file, buffer, p.fieldName, p.expected, p.numBytes)) {
            return false;
        }
    }

    // the number of channels
    if (!readInt(file, buffer, "number of channels", 2, f.channels)) {
        return false;
    }

    // sample rate
    if (!readInt(file, buffer, "sample rate", 4, f.sampleRate)) {
        return false;
    }

    // (sampleRate * bitsPerSample * channels) / 8
    if (!readInt(file, buffer, "(sampleRate * bitsPerSample * channels) / 8", 4)) {
        return false;
    }

    // block align
    if (!checkField(file, buffer, "block align", 2)) {
        return false;
    }

    // bitsPerSample
    if (!readInt(file, buffer, "bits per sample", 2, f.bitsPerSample)) {
        return false;
    }

    // data chunk header "data"
    if (!checkField(file, buffer, "data chunk header", "data", 4)) {
        return false;
    }

    // size of data
    if (!readInt(file, buffer, "data size", 4, f.size)) {
        return false;
    }

    // check file state
    if (file.eof()) {
        std::cerr << "Error: reached EOF on the file" << std::endl;
        return false;
    }

    if (file.fail()) {
        std::cerr << "Error: fail state set on the file" << std::endl;
        return false;
    }

    return true;
}

} // end of anonymous namespace

namespace nuaudio
{
bool loadWav(const std::filesystem::path& filename, WavFile& file)
{
    std::ifstream in(filename.string(), std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return false;
    }

    if (!loadWavFileHeader(in, file)) {
        std::cerr << "Error: Could not load wav header of " << filename << std::endl;
        return false;
    }

    file.data.resize(file.size);
    in.read(&file.data[0], file.size);
    return true;
}
}
