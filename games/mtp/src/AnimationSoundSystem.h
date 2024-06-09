#pragma once

#include <filesystem>
#include <random>
#include <vector>

#include "Events.h"

class AudioManager;
class EventManager;
class Camera;
class IAudioManager;

class AnimationSoundSystem {
public:
    void init(
        EventManager& em,
        IAudioManager& audioManager,
        const std::filesystem::path& soundsPath);
    void cleanup(EventManager& em);

    void update(entt::registry& registry, const Camera& camera, float dt);

private:
    void onAnimationEvent(const EntityAnimationEvent& event);

    void handleStepSounds(IAudioManager& am, const entt::handle& e, const std::string& soundName);

    std::vector<EntityAnimationEvent> queuedEvents;

    std::mt19937 randomEngine;
    IAudioManager* audioManager{nullptr};

    std::filesystem::path soundsPath;
};
