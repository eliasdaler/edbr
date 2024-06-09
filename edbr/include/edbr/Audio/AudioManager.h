#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <edbr/Audio/Music.h>
#include <edbr/Audio/Sound.h>
#include <edbr/Audio/SoundDevice.h>

#include <edbr/Audio/IAudioManager.h>

class SoundCache {
public:
    void cleanup();

    std::shared_ptr<nuaudio::SoundBuffer> loadOrGetSound(const std::filesystem::path& path);

private:
    std::unordered_map<std::filesystem::path, std::shared_ptr<nuaudio::SoundBuffer>> sounds;
};

class AudioManager : public IAudioManager {
public:
    bool init() override;
    void cleanup() override;

    void loadSettings(const std::filesystem::path& settingsPath) override;
    void setMusicDirectory(const std::filesystem::path& dir) override { musicDirectory = dir; }
    void setMusicExt(const std::string& ext) override { musicExt = ext; }

    void update() override;

    // sound
    bool playSound(const std::string& name) override;
    bool playSound(const std::string& name, float x, float y, float z, float pitch = 1.f) override;
    void stopSound(const std::string& soundTag) override;

    // music
    void playMusic(const std::string& name) override;
    void setMusicVolume(const std::string& name, float value) override;
    void resumeMusic(const std::string& name) override;
    void pauseMusic(const std::string& name) override;
    void stopMusic(const std::string& name) override;
    bool isMusicPlaying(const std::string& name) const override;
    float getMusicPosition() const override;

    // listener
    void setListenerPosition(float x, float y, float z) override;
    // orientation = {f_x, f_y, f_z, u_x, u_y, u_z}, where f - forward vector, u - up vector
    void setListenerOrientation(const std::array<float, 6>& orientation) override;

    void pauseAllSound(bool pauseMusic = true) override;
    void resumeAllSound() override;

    void setMuted(bool muted) override;
    bool isMuted() const override;

private:
    // default stuff
    float defaultVolume = 100.f;
    float defaultPitch = 1.f;
    float defaultReferenceDistance = 7.5f;
    float defaultMaxDistance = 100.f;
    float defaultRolloffFactor = 1.f;

    SoundDevice device;
    SoundCache soundCache;

    std::filesystem::path musicDirectory;
    std::string musicExt;

    struct SoundInfo {
        std::unique_ptr<nuaudio::Sound> sound;
        std::string tag;
        bool isAmbient{false};
    };
    std::vector<SoundInfo> sounds;
    std::vector<SoundInfo> soundQueue;

    std::unordered_map<std::string, nuaudio::Music> musics;
    nuaudio::Music music;
};
