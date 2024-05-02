#include "Level.h"

#include <edbr/Core/JsonFile.h>

void Level::load(const std::filesystem::path& path, GfxDevice& gfxDevice)
{
    this->path = path;
    JsonFile file(path);
    if (!file.isGood()) {
        throw std::runtime_error(fmt::format("failed to load tilemap from {}", path.string()));
    }
    const auto& loader = file.getLoader();

    spawners.clear();
    if (loader.hasKey("entities")) {
        auto spawnerLoaders = loader.getLoader("entities").getVector();
        spawners.reserve(spawnerLoaders.size());

        for (const auto& sl : spawnerLoaders) {
            Spawner spawner;
            sl.get("prefab", spawner.prefabName);
            sl.get("pos", spawner.position);
            if (sl.hasKey("components")) {
                spawner.prefabData = sl.getLoader("components").getJson();
            }
            if (sl.hasKey("tag")) {
                sl.get("tag", spawner.tag);
            }
            if (sl.hasKey("dir")) {
                std::string dir;
                sl.get("dir", dir);
                if (dir == "Left") {
                    spawner.heading = {-1.f, 0.f};
                } else if (dir == "Right") {
                    spawner.heading = {1.f, 0.f};
                } else {
                    throw std::runtime_error(fmt::format("invalid direction: {}", dir));
                }
            } else {
                spawner.heading = {1.f, 0.f};
            }

            spawners.push_back(std::move(spawner));
        }
    }

    if (loader.hasKey("tilemap")) {
        tileMap.load(loader.getLoader("tilemap"), gfxDevice);
    }

    auto s = path.stem();
    s.replace_extension("");
    name = s.string();
}
