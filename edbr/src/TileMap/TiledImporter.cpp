#include <edbr/TileMap/TiledImporter.h>

#include <edbr/Core/JsonFile.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/TileMap/TileMap.h>

namespace
{
const unsigned FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
const unsigned FLIPPED_VERTICALLY_FLAG = 0x40000000;
const unsigned FLIPPED_DIAGONALLY_FLAG = 0x20000000;
const unsigned ROTATED_HEXAGONAL_120_FLAG = 0x10000000;
}

void TiledImporter::loadLevel(
    GfxDevice& gfxDevice,
    const std::filesystem::path& path,
    TileMap& tileMap)
{
    JsonFile file(path);
    if (!file.isGood()) {
        throw std::runtime_error(fmt::format("failed to load level from {}", path.string()));
    }

    const auto basePath = path.parent_path();

    const auto& loader = file.getLoader();

    // load tilesets
    for (const auto& tilesetLoader : loader.getLoader("tilesets").getVector()) {
        std::uint32_t firstGID;
        std::filesystem::path tilesetPath;
        tilesetLoader.get("firstgid", firstGID);
        tilesetLoader.get("source", tilesetPath);
        tilesets.push_back(loadTileset(basePath / tilesetPath, firstGID));
    }
    assert(!tilesets.empty());

    // add tilesets to tilemap
    for (std::size_t i = 0; i < tilesets.size(); ++i) {
        const auto& tiledTS = tilesets[i];
        auto tileset = TileMap::Tileset{
            .name = tiledTS.name,
            .texture = gfxDevice.loadImageFromFile(tiledTS.imagePath),
            .textureSize = tiledTS.imageSize,
        };
        tileset.animations.reserve(tiledTS.animations.size());
        for (const auto& anim : tiledTS.animations) {
            assert(!anim.tileIds.empty());
            auto& animation = tileset.animations[anim.tileIds[0]];
            animation.duration = anim.duration;
            animation.tileIds.reserve(animation.tileIds.size());
            for (const auto& tid : anim.tileIds) {
                animation.tileIds.push_back(static_cast<TileMap::TileId>(tid));
            }
        }
        tileMap.addTileset((TileMap::TilesetId)i, std::move(tileset));
    }

    // load tile layers
    for (const auto& layerLoader : loader.getLoader("layers").getVector()) {
        std::string layerType;
        layerLoader.get("type", layerType);
        if (layerType != "tilelayer") {
            continue;
        }

        std::string layerName;
        layerLoader.get("name", layerName);

        // get layer Z
        int layerZ = 0;
        for (const auto& propLoader : layerLoader.getLoader("properties").getVector()) {
            std::string propName;
            propLoader.get("name", propName);
            if (propName == "z") {
                propLoader.get("value", layerZ);
                break;
            }
        }

        auto& layer = tileMap.addLayer(TileMap::TileMapLayer{
            .name = layerName,
            .z = layerZ,
        });

        for (const auto& chunkLoader : layerLoader.getLoader("chunks").getVector()) {
            int chunkX, chunkY;
            chunkLoader.get("x", chunkX);
            chunkLoader.get("y", chunkY);

            int chunkWidth;
            chunkLoader.get("width", chunkWidth);
            assert(chunkWidth != 0);

            auto& data = chunkLoader.getLoader("data").getJson();
            assert(data.is_array());

            for (std::size_t arrIdx = 0; arrIdx < data.size(); ++arrIdx) {
                std::uint32_t tileGID = data[arrIdx];

                bool flippedHorizontally = (tileGID & FLIPPED_HORIZONTALLY_FLAG);
                bool flippedVertically = (tileGID & FLIPPED_VERTICALLY_FLAG);
                bool flippedDiagonally = (tileGID & FLIPPED_DIAGONALLY_FLAG);
                bool rotatedHex120 = (tileGID & ROTATED_HEXAGONAL_120_FLAG);

                // Clear all four flags
                tileGID &=
                    ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG |
                      FLIPPED_DIAGONALLY_FLAG | ROTATED_HEXAGONAL_120_FLAG);
                if (tileGID == 0) {
                    continue;
                }

                auto tilesetIdx = findTileset(tileGID);
                auto tileID = tileGID - tilesets[tilesetIdx].firstGID;

                int chunkLocalX = arrIdx % chunkWidth;
                int chunkLocalY = arrIdx / chunkWidth;

                int tileX = chunkX + chunkLocalX;
                int tileY = chunkY + chunkLocalY;

                layer.tiles.emplace(
                    TileMap::TileIndex{tileX, tileY},
                    TileMap::Tile{
                        .id = (TileMap::TileId)tileID,
                        .tilesetId = (TileMap::TilesetId)tilesetIdx,
                    });
            }
        }
    }

    // TODO: move somewhere else?
    tileMap.updateAnimatedTileIndices();

    // load object layers
    for (const auto& layerLoader : loader.getLoader("layers").getVector()) {
        std::string layerType;
        layerLoader.get("type", layerType);
        if (layerType != "objectgroup") {
            continue;
        }

        for (const auto& objectLoader : layerLoader.getLoader("objects").getVector()) {
            TiledObject object;
            objectLoader.get("x", object.position.x);
            objectLoader.get("y", object.position.y);
            objectLoader.get("width", object.size.x);
            objectLoader.get("height", object.size.y);
            objectLoader.get("name", object.name);
            objectLoader.get("type", object.className);

            if (objectLoader.hasKey("properties")) {
                for (const auto& propLoader : objectLoader.getLoader("properties").getVector()) {
                    std::string propName;
                    std::string propType;
                    propLoader.get("name", propName);
                    propLoader.get("type", propType);

                    CustomProperty property{.name = propName};
                    if (propType == "string") {
                        std::string value;
                        propLoader.get("value", value);
                        property.value = std::move(value);
                    }

                    object.customProperties.push_back(std::move(property));
                }
            }

            objects.push_back(std::move(object));
        }
    }
}

TiledImporter::TiledTileset TiledImporter::loadTileset(
    const std::filesystem::path& path,
    std::uint32_t firstGID)
{
    const auto basePath = path.parent_path();

    JsonFile file(path);
    if (!file.isGood()) {
        throw std::runtime_error(fmt::format("failed to load level from {}", path.string()));
    }

    const auto& loader = file.getLoader();
    std::string name;
    std::filesystem::path tilesetPath;
    int imageWidth, imageHeight;

    loader.get("name", name);
    loader.get("image", tilesetPath);
    loader.get("imagewidth", imageWidth);
    loader.get("imageheight", imageHeight);

    auto tileset = TiledTileset{
        .name = name,
        .firstGID = firstGID,
        .imagePath = basePath / tilesetPath,
        .imageSize = {imageWidth, imageHeight},
    };

    if (loader.hasKey("tiles")) {
        const auto tileAnimLoaders = loader.getLoader("tiles").getVector();
        tileset.animations.reserve(tileAnimLoaders.size());
        for (const auto& tileAnimLoader : tileAnimLoaders) {
            std::uint32_t firstTileId{0};
            tileAnimLoader.get("id", firstTileId);

            TiledTileset::TileAnimation animation{};
            const auto frameLoaders = tileAnimLoader.getLoader("animation").getVector();
            animation.tileIds.reserve(frameLoaders.size());
            for (const auto& frameLoader : frameLoaders) {
                float duration{};
                frameLoader.get("duration", duration);
                duration /= 1000; // from ms to seconds

                std::uint32_t tileId;
                frameLoader.get("tileid", tileId);

                if (animation.duration == 0) {
                    animation.duration = duration;
                    assert(tileId == firstTileId && "first 'tileid' should be equal to 'id'");
                } else {
                    assert(
                        duration == animation.duration &&
                        "tile animation should have constant frame duration");
                }

                animation.tileIds.push_back(tileId);
            }
            tileset.animations.push_back(std::move(animation));
        }
    }

    return tileset;
}

std::size_t TiledImporter::findTileset(const std::uint32_t tileGID) const
{
    assert(!tilesets.empty());
    for (int i = static_cast<int>(tilesets.size()) - 1; i >= 0; --i) {
        const auto& tileset = tilesets[i];

        if (tileset.firstGID <= tileGID) {
            return i;
        }
    }
    assert(false);
    return 0;
}
