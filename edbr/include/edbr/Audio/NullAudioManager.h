#pragma once

#include <edbr/Audio/IAudioManager.h>

// NullAudioManager is used in situations when audio has failed to be initialized
class NullAudioManager : public IAudioManager {
public:
    bool init() override { return true; }
    void cleanup() override {}

    void loadSettings(const std::filesystem::path& settingsPath) override {}
    void setMusicDirectory(const std::filesystem::path& dir) override {}
    void setMusicExt(const std::string& ext) override {}

    void update() override {}

    // sound
    bool playSound(const std::string& name) override { return true; }
    bool playSound(const std::string& name, float x, float y, float z, float pitch) override
    {
        return true;
    }
    void stopSound(const std::string& soundTag) override {}

    // music
    void playMusic(const std::string& name) override {}
    void setMusicVolume(const std::string& name, float value) override {}
    void resumeMusic(const std::string& name) override {}
    void pauseMusic(const std::string& name) override {}
    void stopMusic(const std::string& name) override {}
    bool isMusicPlaying(const std::string& name) const override { return false; }
    float getMusicPosition() const override { return 0.f; }

    // listener
    void setListenerPosition(float x, float y, float z) override {}
    void setListenerOrientation(const std::array<float, 6>& orientation) override {}

    void pauseAllSound(bool pauseMusic) override {}
    void resumeAllSound() override {}

    void setMuted(bool muted) override {}
    bool isMuted() const override { return false; }
};
