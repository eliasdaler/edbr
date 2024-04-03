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

class SoundCache {
public:
    void exit();

    std::shared_ptr<nuaudio::SoundBuffer> loadOrGetSound(const std::filesystem::path& path);

private:
    std::unordered_map<std::filesystem::path, std::shared_ptr<nuaudio::SoundBuffer>> sounds;
};

class AudioManager {
public:
    void exit();

    void loadSettings(const std::filesystem::path& settingsPath);

    bool playSound(const std::string& name);
    bool playSound(const std::string& name, float x, float y, float z, float pitch = 1.f);

    void stopSound(const std::string& soundTag);

    void playMusic(const std::string& name);
    void setMusicVolume(const std::string& name, float value);

    void resumeMusic(const std::string& name);
    void pauseMusic(const std::string& name);
    void stopMusic(const std::string& name);
    bool isMusicPlaying(const std::string& name) const;
    float getMusicPosition() const;

    void setListenerPosition(float x, float y, float z);
    // orientation = {f_x, f_y, f_z, u_x, u_y, u_z}, where f - forward vector, u - up vector
    void setListenerOrientation(const std::array<float, 6>& orientation);

    void update();

    void pauseAllSound(bool pauseMusic = true);
    void resumeAllSound();

    void setMuted(bool muted);
    bool isMuted() const;

    void setMusicDirectory(const std::filesystem::path& dir) { musicDirectory = dir; }
    void setMusicExt(const std::string& ext) { musicExt = ext; }

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
    std::unordered_map<std::string, nuaudio::Music> musics;
    nuaudio::Music music;

    struct SoundInfo {
        std::unique_ptr<nuaudio::Sound> sound;
        std::string tag;
        bool isAmbient{false};
    };
    std::vector<SoundInfo> sounds;
    std::vector<SoundInfo> soundQueue;
};
