#include <edbr/AudioManager.h>

#include <edbr/Audio/OpenALUtil.h>
#include <edbr/Audio/SoundBuffer.h>

#include <edbr/Core/JsonFile.h>

#include <fmt/printf.h>

#define EDBR_LOG_ERROR fmt::println

void SoundCache::exit()
{
    sounds.clear(); // need to do this before destroying OpenAL device
                    // (before AudioManager gets destroyed)
}

std::shared_ptr<nuaudio::SoundBuffer> SoundCache::loadOrGetSound(const std::filesystem::path& path)
{
    auto it = sounds.find(path);
    if (it != sounds.end()) {
        return it->second;
    }

    auto sound = std::make_shared<nuaudio::SoundBuffer>();
    if (!sound->loadFromFile(path)) {
        fmt::print("Failed to load sound {}\n", path.string());
        return nullptr;
    }
    auto it2 = sounds.insert_or_assign(path, std::move(sound));
    return it2.first->second;
}

void AudioManager::exit()
{
    for (auto& sound : sounds) {
        sound.sound->stop();
    }
    sounds.clear();
    soundQueue.clear();

    for (auto& [name, music] : musics) {
        music.stop();
    }

    musics.clear();
    soundCache.exit();
}

void AudioManager::loadSettings(const std::filesystem::path& settingsPath)
{
    JsonFile file(settingsPath);
    if (!file.isGood()) {
        throw std::runtime_error(
            fmt::format("failed to load audio settings from {}", settingsPath.string()));
    }
    const auto loader = file.getLoader();

    loader.get("defaultVolume", defaultVolume);
    loader.get("defaultPitch", defaultPitch);
    loader.get("defaultReferenceDistance", defaultReferenceDistance);
    loader.get("defaultMaxDistance", defaultMaxDistance);
    loader.get("defaultRolloffFactor", defaultRolloffFactor);
}

bool AudioManager::playSound(const std::string& name)
{
    auto buffer = soundCache.loadOrGetSound(name);
    if (buffer) {
        auto sound = std::make_unique<nuaudio::Sound>();
        sound->setBuffer(*buffer);
        sound->setRolloffFactor(0.f);
        if (device.hasReverb()) {
            sound->setEffectSlot(device.getEffectSlot());
        }
        soundQueue.push_back(SoundInfo{std::move(sound), "", false});
        return true;
    }
    return false;
}

bool AudioManager::playSound(const std::string& name, float x, float y, float z, float pitch)
{
    auto buffer = soundCache.loadOrGetSound(name);
    if (buffer) {
        auto sound = std::make_unique<nuaudio::Sound>();
        sound->setBuffer(*buffer);
        sound->setPosition(x, y, z);
        sound->setRolloffFactor(defaultRolloffFactor);
        sound->setReferenceDistance(defaultReferenceDistance);
        sound->setMaxDistance(defaultMaxDistance);
        sound->setPitch(pitch);
        if (device.hasReverb()) {
            sound->setEffectSlot(device.getEffectSlot());
        }
        soundQueue.push_back(SoundInfo{std::move(sound), "", false});
        return true;
    }
    return false;
}

void AudioManager::stopSound(const std::string& soundTag)
{
    auto it = std::find_if(sounds.begin(), sounds.end(), [&soundTag](const auto& elem) {
        return elem.tag == soundTag;
    });
    if (it != sounds.end()) {
        it->sound->stop();
        sounds.erase(it);
    }
}

void AudioManager::playMusic(const std::string& name)
{
    auto it = musics.find(name);
    if (it != musics.end()) {
        EDBR_LOG_ERROR("[audio] song {} is already playing", name);
        return;
    }

    auto& music = musics[name];
    const std::filesystem::path musicPath{name};
    if (!music.openFromFile(musicPath)) {
        EDBR_LOG_ERROR("[audio] song {} failed to open", musicPath.string());
        auto it = musics.find(name);
        musics.erase(it);
        return;
    }

    if (music.getStatus() != nuaudio::Music::Status::Playing) {
        music.setLooped(true); // TODO: why?
        music.play();
    }
}

void AudioManager::setMusicVolume(const std::string& name, float value)
{
    auto it = musics.find(name);
    if (it != musics.end()) {
        it->second.setVolume(value);
        // return;
    } else {
        EDBR_LOG_ERROR("[audio] song {} is not playing\n", name);
    }
}

void AudioManager::resumeMusic(const std::string& name)
{
    if (auto it = musics.find(name); it != musics.end()) {
        it->second.play();
    } else {
        EDBR_LOG_ERROR("[audio] song with tag {} is not found\n", name);
    }
}

void AudioManager::pauseMusic(const std::string& name)
{
    if (auto it = musics.find(name); it != musics.end()) {
        it->second.pause();
    } else {
        EDBR_LOG_ERROR("[audio] song with tag {} is not found\n", name);
    }
}

void AudioManager::stopMusic(const std::string& name)
{
    if (auto it = musics.find(name); it != musics.end()) {
        it->second.stop();
        musics.erase(it);
    } else {
        EDBR_LOG_ERROR("[audio] song with tag {} is not found\n", name);
    }
}

bool AudioManager::isMusicPlaying(const std::string& name) const
{
    if (auto it = musics.find(name); it != musics.end()) {
        return it->second.getStatus() == nuaudio::Music::Status::Playing;
    }
    return false;
}

float AudioManager::getMusicPosition() const
{
    return 0.f;
}

void AudioManager::setListenerPosition(float x, float y, float z)
{
    util::setOpenALListenerPosition(x, y, z);
}

void AudioManager::setListenerOrientation(const std::array<float, 6>& orientation)
{
    util::setOpenALListenerOrientation(orientation);
}

void AudioManager::update()
{
    for (auto& sound : soundQueue) {
        sounds.push_back(std::move(sound));
        sounds.back().sound->play();
    }

    soundQueue.clear();

    // remove sounds which stopped playing
    std::erase_if(sounds, [](const auto& elem) {
        return elem.sound->getStatus() == nuaudio::Sound::Status::Stopped;
    });

    // update music
    // TODO: do it in separate thread?
    for (auto& [name, music] : musics) {
        if (music.getStatus() == nuaudio::Music::Status::Playing) {
            music.update();
        }
    }
}

void AudioManager::pauseAllSound(bool pauseMusic)
{
    if (pauseMusic) {
        for (auto& [name, music] : musics) {
            if (music.getStatus() == nuaudio::Music::Status::Playing) {
                music.pause();
            }
        }
    }

    for (auto& soundInfo : sounds) {
        auto& sound = *soundInfo.sound;
        if (sound.getStatus() == nuaudio::Sound::Status::Playing) {
            sound.pause();
        }
    }
}

void AudioManager::resumeAllSound()
{
    for (auto& [name, music] : musics) {
        if (music.getStatus() == nuaudio::Music::Status::Paused) {
            music.play();
        }
    }

    for (auto& soundInfo : sounds) {
        auto& sound = *soundInfo.sound;
        if (sound.getStatus() == nuaudio::Sound::Status::Paused) {
            sound.play();
        }
    }
}

void AudioManager::setMuted(bool muted)
{
    alCall(alListenerf, AL_GAIN, muted ? 0.f : 1.f); // TODO: restore previous volume
}

bool AudioManager::isMuted() const
{
    ALfloat volume;
    alCall(alGetListenerf, AL_GAIN, &volume);
    return volume == 0.f;
}
