#pragma once

#include <filesystem>
#include <random>
#include <vector>

#include "Events.h"

class AudioManager;
class EventManager;
class Camera;

class AnimationSoundSystem {
public:
    AnimationSoundSystem(AudioManager& audioManager);
    void init(EventManager& em, const std::filesystem::path& soundsPath);
    void cleanup(EventManager& em);

    void update(entt::registry& registry, const Camera& camera, float dt);

private:
    void onAnimationEvent(const EntityAnimationEvent& event);

    void handleStepSounds(AudioManager& am, const entt::handle& e, const std::string& soundName);

    std::vector<EntityAnimationEvent> queuedEvents;

    std::mt19937 randomEngine;
    AudioManager& audioManager;

    std::filesystem::path soundsPath;
};
