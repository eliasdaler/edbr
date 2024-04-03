#pragma once

#include <filesystem>
#include <memory>

namespace nuaudio
{

class Music {
public:
    struct StreamingAudioData;
    enum class Status { Stopped, Playing, Paused };

    Music();
    ~Music();

    // TODO: implement some of them?
    Music(const Music&) = delete;
    Music(Music&&) = delete;
    Music& operator=(const Music&) = delete;
    Music& operator=(Music&&) = delete;

    bool openFromFile(const std::filesystem::path& filename);
    void play();
    void pause();
    void update();
    void stop();

    void setLooped(bool b);
    bool isLooped() const;
    Status getStatus() const;

    void setVolume(float volume);

private:
    std::unique_ptr<StreamingAudioData> audioData;
    bool looped{true};
};
}
