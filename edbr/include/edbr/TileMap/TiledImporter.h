#pragma once

#include <filesystem>
#include <string>
#include <variant>
#include <vector>

#include <glm/vec2.hpp>

class GfxDevice;
class TileMap;

class TiledImporter {
public:
    struct CustomProperty {
        std::string name;
        std::variant<int, std::string> value;
    };

    struct TiledObject {
        glm::vec2 position;
        std::string name;
        std::string className;
        std::vector<CustomProperty> customProperties;
        glm::vec2 size;
    };

public:
    void loadLevel(GfxDevice& gfxDevice, const std::filesystem::path& path, TileMap& tileMap);

    const std::vector<TiledObject>& getObjects() const { return objects; }

private:
    struct TiledTileset {
        std::string name;
        std::uint32_t firstGID;
        std::filesystem::path imagePath;
        glm::ivec2 imageSize;
    };

    TiledTileset loadTileset(const std::filesystem::path& path, std::uint32_t firstGID);

    std::size_t findTileset(std::uint32_t tileGID) const;

    std::vector<TiledTileset> tilesets;
    std::vector<TiledObject> objects;
};
