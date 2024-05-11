#include <edbr/TileMap/TileMap.h>

#include <edbr/Core/JsonDataLoader.h>
#include <edbr/Graphics/GfxDevice.h>

#include <fmt/format.h>

#include <limits>
#include <stdexcept>

#include <glm/gtx/string_cast.hpp>

namespace
{
static constexpr auto tileImageSize = 16;
static constexpr auto tileWorldSize = glm::vec2{16.f};

glm::vec2 pixelCoordToUV(const glm::ivec2& pixelCoord, const glm::ivec2& textureSize)
{
    return static_cast<glm::vec2>(pixelCoord) / static_cast<glm::vec2>(textureSize);
}

} // end of anonymous namespace

namespace edbr::tilemap
{
glm::vec2 tileIndexToWorldPos(const TileMap::TileIndex& tileIndex)
{
    return static_cast<glm::vec2>(tileIndex) * tileWorldSize;
}

std::pair<glm::vec2, glm::vec2> tileIdToUVs(
    TileMap::TileId tileId,
    const glm::ivec2& tilesetTextureSize)
{
    const auto numTilesX = tilesetTextureSize.x / tileImageSize;
    const auto tilePixelCoords = glm::ivec2{tileId % numTilesX, tileId / numTilesX} * tileImageSize;

    const auto uv1 = pixelCoordToUV(tilePixelCoords, tilesetTextureSize);
    const auto uv2 =
        pixelCoordToUV(tilePixelCoords + glm::ivec2{tileImageSize}, tilesetTextureSize);

    return {uv1, uv2};
}
}

void TileMap::clear()
{
    layers.clear();
    tilesets.clear();
}

void TileMap::load(const JsonDataLoader& loader, GfxDevice& gfxDevice)
{
    clear();

    auto tilesetIds = loader.getLoader("tilesets").getKeyValueMapInt();
    tilesets.reserve(tilesetIds.size());
    for (const auto& [tilesetName, tilesetId] : tilesetIds) {
        Tileset tileset;
        tileset.name = tilesetName;
        // FIXME: don't hardcode tilesets dir
        tileset.texture = gfxDevice.loadImageFromFile("assets/tilesets/" + tilesetName + ".png");
        tilesets.emplace(tilesetId, std::move(tileset));
    }

    // load layers
    const auto layersLoaders = loader.getLoader("layers").getVector();
    layers.reserve(layersLoaders.size());
    for (const auto& layerLoader : layersLoaders) {
        TileMapLayer layer;
        layerLoader.get("name", layer.name);
        layerLoader.get("z", layer.z);

        // load tiles
        if (!layerLoader.hasKey("tiles")) {
            continue;
        }

        const auto tilesLoaders = layerLoader.getLoader("tiles").getVector();
        layer.tiles.reserve(tilesLoaders.size());
        for (const auto& tileLoader : tilesLoaders) {
            glm::vec2 tileIndex;
            tileLoader.get("i", tileIndex);

            Tile tile{};
            tileLoader.get("id", tile.id);
            tileLoader.get("tid", tile.tilesetId);
            layer.tiles.emplace(tileIndex, std::move(tile));
        }

        layers.push_back(std::move(layer));
    }
}

int TileMap::getMinLayerZ() const
{
    auto minZ = std::numeric_limits<int>::max();
    for (const auto& l : layers) {
        if (l.z < minZ) {
            minZ = l.z;
        }
    }
    return minZ;
}

int TileMap::getMaxLayerZ() const
{
    auto maxZ = std::numeric_limits<int>::min();
    for (const auto& l : layers) {
        if (l.z > maxZ) {
            maxZ = l.z;
        }
    }
    return maxZ;
}

TileMap::TileMapLayer& TileMap::addLayer(TileMapLayer layer)
{
    for (const auto& l : layers) {
        if (l.name == layer.name) {
            throw std::runtime_error(
                fmt::format("layer with name '{}' was already added", layer.name));
        }
    }
    layers.push_back(std::move(layer));
    return layers.back();
}

const TileMap::TileMapLayer& TileMap::getLayer(const std::string& name) const
{
    for (const auto& l : layers) {
        if (l.name == name) {
            return l;
        }
    }
    throw std::runtime_error(fmt::format("layer with name '{}' was not created", name));
}

bool TileMap::layerExists(int z) const
{
    for (const auto& l : layers) {
        if (l.z == z) {
            return true;
        }
    }
    return false;
}

const TileMap::Tile& TileMap::TileMapLayer::getTile(const TileIndex& tileIndex) const
{
    static Tile emptyTile{.id = NULL_TILE_ID, .tilesetId = NULL_TILESET_ID};

    auto it = tiles.find(tileIndex);
    if (it == tiles.end()) {
        return emptyTile;
    }
    return it->second;
}

const TileMap::Tile& TileMap::getTile(const std::string& layerName, const TileIndex& tileIndex)
    const
{
    return getLayer(layerName).getTile(tileIndex);
}

TileMap::TileIndex TileMap::GetTileIndex(const glm::vec2& worldPos)
{
    return static_cast<glm::ivec2>(glm::floor(worldPos / tileWorldSize));
}

math::FloatRect TileMap::GetTileAABB(const TileIndex& tileIndex)
{
    return {static_cast<glm::vec2>(tileIndex) * tileWorldSize, tileWorldSize};
}

math::IndexRange2 TileMap::getTileIndicesInRect(const math::FloatRect& rect) const
{
    assert(rect.width >= 0.f && rect.height >= 0.f);
    const auto leftTopTileIndex = GetTileIndex(rect.getTopLeftCorner());

    // substract 0.1 for edge condition, when rect bottom right is at the br-corner of the tile
    // this code is precise up to ~16000000.f
    const auto rightDownTileIndex =
        GetTileIndex(rect.getBottomRightCorner() - glm::vec2{0.1f, 0.1f});
    const auto numberOfTiles = (rightDownTileIndex - leftTopTileIndex) + glm::ivec2{1, 1};

    return math::IndexRange2(leftTopTileIndex, numberOfTiles);
}

void TileMap::addTileset(TilesetId tilesetId, Tileset tileset)
{
    assert(tileset.texture != NULL_IMAGE_ID);
    assert(!tilesets.contains(tilesetId));
    tilesets.emplace(tilesetId, std::move(tileset));
}

ImageId TileMap::getTilesetImageId(TileMap::TilesetId tilesetId) const
{
    return tilesets.at(tilesetId).texture;
}
