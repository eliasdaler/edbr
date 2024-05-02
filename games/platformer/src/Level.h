#pragma once

#include <filesystem>
#include <string>

#include <edbr/TileMap/TileMap.h>

#include "Spawner.h"

class LevelState;

class Level {
public:
    void load(const std::filesystem::path& path, GfxDevice& gfxDevice);

    const TileMap& getTileMap() const { return tileMap; }
    const std::vector<Spawner>& getSpawners() const { return spawners; }

    const std::filesystem::path& getPath() const { return path; }
    const std::string& getName() const { return name; }

    LevelState* state{nullptr};

private:
    std::filesystem::path path;
    std::string name;
    TileMap tileMap;

    std::vector<Spawner> spawners;
};
