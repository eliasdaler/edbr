#include "AnimationSoundSystem.h"

#include <edbr/AudioManager.h>
#include <edbr/ECS/Components/MetaInfoComponent.h>
#include <edbr/Event/EventManager.h>
#include <edbr/Graphics/Camera.h>

#include "Components.h"
#include "EntityUtil.h"
#include "Events.h"

#include <fmt/format.h>

AnimationSoundSystem::AnimationSoundSystem(AudioManager& audioManager) : audioManager(audioManager)
{}

void AnimationSoundSystem::init(EventManager& em, const std::filesystem::path& soundsPath)
{
    assert(std::filesystem::exists(soundsPath));
    this->soundsPath = soundsPath;

    em.addListener(this, &AnimationSoundSystem::onAnimationEvent);
}

void AnimationSoundSystem::cleanup(EventManager& em)
{
    em.removeListener<EntityAnimationEvent>(this);
}

void AnimationSoundSystem::update(entt::registry& registry, const Camera& camera, float dt)
{
    { // update listener transform
        const auto listenerTransform = camera.getTransform();
        audioManager.setListenerPosition(
            listenerTransform.getPosition().x,
            listenerTransform.getPosition().y,
            listenerTransform.getPosition().z);
        const auto f = listenerTransform.getLocalFront();
        const auto u = listenerTransform.getLocalUp();
        audioManager.setListenerOrientation({f.x, f.y, f.z, u.x, u.y, u.z});
    }

    for (const auto& event : queuedEvents) {
        assert(event.entity.valid());
        auto soundName = event.event;
        if (const auto ascPtr = event.entity.try_get<AnimationEventSoundComponent>(); ascPtr) {
            if (!soundName.empty()) {
                soundName = ascPtr->getSoundName(event.event);
            }
        }
        if (soundName.starts_with("step")) {
            handleStepSounds(audioManager, event.entity, soundName);
        } else {
            // TODO: sound component?
            // e.g. to be able to map "step1" to "cat_step1"
            const auto soundPath = soundsPath / (soundName + ".wav");
            const auto& pos = entityutil::getWorldPosition(event.entity);
            audioManager.playSound(soundPath, pos.x, pos.y, pos.z);
        }
    }
    queuedEvents.clear();
}

void AnimationSoundSystem::onAnimationEvent(const EntityAnimationEvent& event)
{
    queuedEvents.push_back(event);
}

void AnimationSoundSystem::handleStepSounds(
    AudioManager& am,
    const entt::handle& e,
    const std::string& soundName)
{
    // TODO: determine somehow by doing ray trace from character's feet
    const std::string material = "concrete";
    const auto soundPath = soundsPath / "steps" / material / (soundName + ".wav");

    auto pitchMin = 0.9f;
    auto pitchMax = 1.f;
    if (e.get<MetaInfoComponent>().prefabName == "cato") {
        // HACK: should be in sound component ideally, e.g.
        // "step": { pitchMin: 0.7f, "pitchMax": 0.85 }
        pitchMin = 0.7f;
        pitchMax = 0.85f;
    }
    std::uniform_real_distribution<float> dist(pitchMin, pitchMax);

    const auto pitch = dist(randomEngine);
    const auto& pos = entityutil::getWorldPosition(e);

    am.playSound(soundPath, pos.x, pos.y, pos.z, pitch);
}
