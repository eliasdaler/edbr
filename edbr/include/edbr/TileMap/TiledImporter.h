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

        struct TileAnimation {
            // Tiled tile animations can have animations with different
            // frame durations, but we assume constant frame speed for ease
            // of implementation
            float duration{0.f}; // in seconds

            std::vector<std::uint32_t> tileIds;
        };
        std::vector<TileAnimation> animations;
    };

    TiledTileset loadTileset(const std::filesystem::path& path, std::uint32_t firstGID);

    std::size_t findTileset(std::uint32_t tileGID) const;

    std::vector<TiledTileset> tilesets;
    std::vector<TiledObject> objects;
};
