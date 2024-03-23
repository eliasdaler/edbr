#include "Level.h"

#include <edbr/Core/JsonFile.h>

// Blender's defaults
const glm::vec4 Level::DefaultAmbientLightColor = glm::vec4{0.051f, 0.051f, 0.051f, 1.f};
const float Level::DefaultAmbientLightIntensity{1.0f};

Level::Level() :
    ambientLightColor(DefaultAmbientLightColor),
    ambientLightIntensity(DefaultAmbientLightIntensity),
    fogActive(false)
{}

void Level::resetToDefault()
{
    name.clear();

    path.clear();
    sceneModelPath.clear();
    defaultPlayerSpawnerName.clear();

    bgmPath.clear();
    defaultCameraName.clear();

    skyboxName.clear();
    ambientLightColor = DefaultAmbientLightColor;
    ambientLightIntensity = DefaultAmbientLightIntensity;

    fogActive = false;
}

void Level::load(const std::filesystem::path path)
{
    resetToDefault();
    this->path = path;

    JsonFile file(path);
    if (!file.isGood()) {
        throw std::runtime_error(fmt::format("failed to load level from {}", path.string()));
    }
    const auto loader = file.getLoader();

    loader.get("model", sceneModelPath);

    loader.getIfExists("bgm", bgmPath);
    loader.getIfExists("default_camera", defaultCameraName);
    loader.getIfExists("default_spawn", defaultPlayerSpawnerName);

    loader.getIfExists("skybox", skyboxName);
    if (loader.hasKey("ambient")) {
        const auto& lightLoader = loader.getLoader("ambient");
        lightLoader.get("color", ambientLightColor, DefaultAmbientLightColor);
        lightLoader.get("intensity", ambientLightIntensity, DefaultAmbientLightIntensity);
    }

    fogActive = false;
    if (loader.hasKey("fog")) {
        fogActive = true;
        const auto& fogLoader = loader.getLoader("fog");
        fogLoader.get("color", fogColor);
        fogLoader.get("density", fogDensity);
    }
}

void Level::loadFromModel(const std::filesystem::path& path)
{
    resetToDefault();
    this->path = path;
    sceneModelPath = path;
}
