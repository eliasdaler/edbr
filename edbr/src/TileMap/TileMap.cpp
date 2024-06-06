#include <edbr/TileMap/TileMap.h>

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

TileMap::TileId textureRectToFrameId(
    const math::IntRect& textureRect,
    const glm::ivec2& tilesetTextureSize)
{
    const auto i = textureRect.getTopLeftCorner() / glm::ivec2{tileImageSize, tileImageSize};
    const auto numTilesX = tilesetTextureSize.x / tileImageSize;
    return i.x + i.y * numTilesX;
}

} // end of anonymous namespace

namespace edbr::tilemap
{

TileMap::TileIndex worldPosToTileIndex(const glm::vec2& worldPos)
{
    return static_cast<glm::ivec2>(glm::floor(worldPos / tileWorldSize));
}

math::FloatRect getTileAABB(const TileMap::TileIndex& tileIndex)
{
    return {static_cast<glm::vec2>(tileIndex) * tileWorldSize, tileWorldSize};
}

glm::vec2 tileIndexToWorldPos(const TileMap::TileIndex& tileIndex)
{
    return static_cast<glm::vec2>(tileIndex) * tileWorldSize;
}

math::IntRect tileIdToTextureRect(TileMap::TileId tileId, const glm::ivec2& tilesetTextureSize)
{
    const auto numTilesX = tilesetTextureSize.x / tileImageSize;
    const auto tilePixelCoords = glm::ivec2{tileId % numTilesX, tileId / numTilesX} * tileImageSize;
    return math::IntRect{tilePixelCoords, {tileImageSize, tileImageSize}};
}

std::pair<glm::vec2, glm::vec2> tileIdToUVs(
    TileMap::TileId tileId,
    const glm::ivec2& tilesetTextureSize)
{
    const auto textureRect = tileIdToTextureRect(tileId, tilesetTextureSize);

    const auto uv1 = pixelCoordToUV(textureRect.getTopLeftCorner(), tilesetTextureSize);
    const auto uv2 = pixelCoordToUV(textureRect.getBottomRightCorner(), tilesetTextureSize);

    return {uv1, uv2};
}

math::IndexRange2 getTileIndicesInRect(const math::FloatRect& rect)
{
    assert(rect.width >= 0.f && rect.height >= 0.f);
    const auto leftTopTileIndex = edbr::tilemap::worldPosToTileIndex(rect.getTopLeftCorner());

    // substract 0.1 for edge condition, when rect bottom right is at the br-corner of the tile
    const auto rightDownTileIndex =
        edbr::tilemap::worldPosToTileIndex(rect.getBottomRightCorner() - glm::vec2{0.1f, 0.1f});
    const auto numberOfTiles = (rightDownTileIndex - leftTopTileIndex) + glm::ivec2{1, 1};

    return math::IndexRange2(leftTopTileIndex, numberOfTiles);
}

} // end of namespace edbr::tilemap

void TileMap::clear()
{
    layers.clear();
    tilesets.clear();
}

void TileMap::update(float dt)
{
    for (auto& [tilesetId, tileset] : tilesets) {
        for (auto& [tileId, animator] : tileset.tileAnimators) {
            animator.update(dt);
        }
    }

    for (auto& layer : layers) {
        for (const auto& ti : layer.animatedTiles) {
            // potential optimization: only animate visible tiles
            auto& tile = layer.tiles.at(ti.tileIndex);
            const auto& tileset = tilesets.at(tile.tilesetId);
            const auto frame = ti.animator->getFrameRect(*ti.spriteSheet);
            tile.id = textureRectToFrameId(frame, tileset.textureSize);
        }
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

void TileMap::addTileset(TilesetId tilesetId, Tileset tileset)
{
    assert(tileset.texture != NULL_IMAGE_ID);
    assert(tileset.textureSize != glm::ivec2{});
    assert(!tilesets.contains(tilesetId));
    tileset.initAnimations();
    tilesets.emplace(tilesetId, std::move(tileset));
}

void TileMap::Tileset::initAnimations()
{
    for (auto& [firstId, anim] : animations) {
        assert(anim.duration != 0.f);
        assert(!anim.tileIds.empty());

        // make animation
        anim.animation.looped = true;
        anim.animation.startFrame = 0;
        anim.animation.endFrame = anim.tileIds.size() - 1;
        anim.animation.frameDuration = anim.duration;

        // make spritesheet
        anim.spriteSheet.frames.reserve(anim.tileIds.size());
        for (std::size_t i = 0; i < anim.tileIds.size(); ++i) {
            const auto& tid = anim.tileIds[i];
            const auto rect = edbr::tilemap::tileIdToTextureRect(tid, textureSize);
            anim.spriteSheet.frames.push_back(rect);
        }

        // create animator
        assert(!tileAnimators.contains(anim.tileIds[0]));
        auto& animator = tileAnimators[anim.tileIds[0]];
        animator.setAnimation(anim.animation, "idle");
    }
}

ImageId TileMap::getTilesetImageId(TileMap::TilesetId tilesetId) const
{
    return tilesets.at(tilesetId).texture;
}

void TileMap::updateAnimatedTileIndices()
{
    // restore tilemap to original state
    for (auto& layer : layers) {
        for (auto& ti : layer.animatedTiles) {
            auto it = layer.tiles.find(ti.tileIndex);
            if (it != layer.tiles.end()) {
                it->second.id = ti.originalTileId;
            }
        }
        layer.animatedTiles.clear();
    }

    // update animated tile list
    for (auto& layer : layers) {
        layer.animatedTiles.clear();
        for (const auto& [ti, tile] : layer.tiles) {
            const auto& tileset = tilesets.at(tile.tilesetId);
            if (tileset.animations.contains(tile.id)) {
                layer.animatedTiles.push_back(TileMapLayer::AnimatedTileInfo{
                    .originalTileId = tile.id,
                    .tileIndex = ti,
                    .animator = &tileset.tileAnimators.at(tile.id),
                    .spriteSheet = &tileset.animations.at(tile.id).spriteSheet,
                });
            }
        }
    }
}
