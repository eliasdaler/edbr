#pragma once

#include <filesystem>
#include <string>

#include <glm/vec4.hpp>

class Level {
public:
    Level();

    // Loads level from a JSON file which describes the level in the following format:
    //
    //   {
    //     "model": "assets/models/levels/xxx.gltf",
    //     "bgm": "assets/music/ambient.ogg",
    //     "default_spawn": "player_spawn",
    //     "default_camera": "default_cam",
    //     "skybox": "assets/images/skybox/daylight",
    //     "ambient": {
    //       "color": { "rgb": [128, 128, 128 ] }, // (or "color": [0.1, 0.1, 0.5] )
    //       "intensity": 0.5
    //     },
    //     "fog": {
    //       "color": { "rgb": [128, 128, 128 ] },
    //       "density": 0.01
    //     }
    //   }
    //
    // All fields except the "model" are optional.
    void load(const std::filesystem::path path);

    // useful for cases where only glTF file exists without a meta-JSON file
    void loadFromModel(const std::filesystem::path& path);

    void setName(const std::string& n) { name = n; }
    const std::string& getName() const { return name; }

    const std::filesystem::path& getSceneModelPath() const { return sceneModelPath; }
    void setSceneModelPath(const std::filesystem::path& p) { sceneModelPath = p; }

    const std::filesystem::path& getBGMPath() const { return bgmPath; }

    const std::string& getDefaultPlayerSpawnerName() const { return defaultPlayerSpawnerName; }

    const std::string& getSkyboxName() const { return skyboxName; }
    bool hasSkybox() const { return !skyboxName.empty(); }

    const glm::vec4& getAmbientLightColor() const { return ambientLightColor; }
    void setAmbientLightColor(const glm::vec4& c) { ambientLightColor = c; }

    float getAmbientLightIntensity() const { return ambientLightIntensity; }
    void setAmbientLightIntensity(float v) { ambientLightIntensity = v; }

    bool isFogActive() const { return fogActive; }
    void setFogActive(bool b) { fogActive = b; }

    const glm::vec4& getFogColor() const { return fogColor; }
    void setFogColor(const glm::vec4& c) { fogColor = c; }

    float getFogDensity() const { return fogDensity; }
    void setFogDensity(float v) { fogDensity = v; }

    const std::filesystem::path& getPath() const { return path; }

    const std::string& getDefaultCameraName() const { return defaultCameraName; }

private:
    void resetToDefault();

    std::string name;

    std::filesystem::path path;
    std::filesystem::path sceneModelPath; // path to a glTF file
    std::string defaultPlayerSpawnerName; // where player spawns if spawn
                                          // destination is not specified

    std::filesystem::path bgmPath;
    std::string defaultCameraName;

    std::string skyboxName;
    glm::vec4 ambientLightColor;
    float ambientLightIntensity{0.f};

    bool fogActive{false};
    glm::vec4 fogColor;
    float fogDensity{0.f};

    static const glm::vec4 DefaultAmbientLightColor;
    static const float DefaultAmbientLightIntensity;
};
