#pragma once

#include <filesystem>
#include <string>

#include <edbr/TileMap/TileMap.h>

#include "Spawner.h"

class Level {
public:
    TileMap& getTileMap() { return tileMap; }
    const TileMap& getTileMap() const { return tileMap; }

    std::vector<Spawner>& getSpawners() { return spawners; }
    const std::vector<Spawner>& getSpawners() const { return spawners; }

    void setName(const std::string& n) { name = n; }
    const std::filesystem::path& getPath() const { return path; }
    const std::string& getName() const { return name; }

private:
    std::filesystem::path path;
    std::string name;
    TileMap tileMap;

    std::vector<Spawner> spawners;
};
