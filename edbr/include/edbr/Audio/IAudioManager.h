#pragma once

#include <array>
#include <filesystem>
#include <string>

class IAudioManager {
public:
    virtual ~IAudioManager() {}

    virtual bool init() = 0;
    virtual void cleanup() = 0;

    virtual void loadSettings(const std::filesystem::path& settingsPath) = 0;
    virtual void setMusicDirectory(const std::filesystem::path& dir) = 0;
    virtual void setMusicExt(const std::string& ext) = 0;

    virtual void update() = 0;

    // sound
    virtual bool playSound(const std::string& name) = 0;
    virtual bool playSound(
        const std::string& name,
        float x,
        float y,
        float z,
        float pitch = 1.f) = 0;
    virtual void stopSound(const std::string& soundTag) = 0;

    // music
    virtual void playMusic(const std::string& name) = 0;
    virtual void setMusicVolume(const std::string& name, float value) = 0;
    virtual void resumeMusic(const std::string& name) = 0;
    virtual void pauseMusic(const std::string& name) = 0;
    virtual void stopMusic(const std::string& name) = 0;
    virtual bool isMusicPlaying(const std::string& name) const = 0;
    virtual float getMusicPosition() const = 0;

    // listener
    virtual void setListenerPosition(float x, float y, float z) = 0;
    // orientation = {f_x, f_y, f_z, u_x, u_y, u_z}, where f - forward vector, u - up vector
    virtual void setListenerOrientation(const std::array<float, 6>& orientation) = 0;

    virtual void pauseAllSound(bool pauseMusic = true) = 0;
    virtual void resumeAllSound() = 0;

    virtual void setMuted(bool muted) = 0;
    virtual bool isMuted() const = 0;
};

// FIXME: this interface is a bit too chunky... it would be nice to separate
// it in a few different interfaces probably
