#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/vec2.hpp>

#include <edbr/Graphics/IdTypes.h>

#include <edbr/Graphics/SpriteAnimation.h>
#include <edbr/Graphics/SpriteAnimator.h>
#include <edbr/Graphics/SpriteSheet.h>
#include <edbr/Math/HashMath.h>
#include <edbr/Math/IndexRange2.h>
#include <edbr/Math/Rect.h>

class JsonDataLoader;
class GfxDevice;

class TileMap {
public:
    using TileId = int;
    static constexpr TileId NULL_TILE_ID = -1;

    using TilesetId = int;
    static constexpr TilesetId NULL_TILESET_ID = -1;

    using TileIndex = glm::ivec2;

    struct Tile {
        TileId id{NULL_TILE_ID};
        TilesetId tilesetId{NULL_TILESET_ID};
    };

    struct TileMapLayer {
        // returns a tile with NULL_TILE_ID and NULL_TILESET_ID if tile is not
        // present on a layer
        const Tile& getTile(const TileIndex& tileIndex) const;

        std::string name;
        int z{false};
        bool isVisible{true};
        std::unordered_map<TileIndex, Tile, math::hash<TileIndex>> tiles;

        struct AnimatedTileInfo {
            TileId originalTileId;
            TileIndex tileIndex;
            const SpriteAnimator* animator{nullptr}; // reference to animator in Tileset
            const SpriteSheet* spriteSheet{nullptr}; // reference to sprite sheet in Tileset
        };
        std::vector<AnimatedTileInfo> animatedTiles;
    };

    struct TilesetAnimation {
        float duration{0}; // in seconds
        std::vector<TileId> tileIds;

        SpriteAnimation animation;
        SpriteSheet spriteSheet;
    };

    struct Tileset {
        void initAnimations();

        std::string name;
        ImageId texture;
        glm::ivec2 textureSize; // to be able to calculate uvs without fetching the image

        std::unordered_map<TileId, TilesetAnimation> animations; // first tile id -> animation
        std::unordered_map<TileId, SpriteAnimator> tileAnimators;
    };

    static constexpr auto CollisionLayerName = "Collision";

public:
    void clear();
    void update(float dt);

    int getMinLayerZ() const;
    int getMaxLayerZ() const;

    TileMap::TileMapLayer& addLayer(TileMapLayer layer);
    const TileMap::TileMapLayer& getLayer(const std::string& name) const;
    const std::vector<TileMap::TileMapLayer>& getLayers() const { return layers; }

    // Returns a tile with NULL_TILE_ID and NULL_TILESET_ID if tile is not
    // present on a layer
    const Tile& getTile(const std::string& layerName, const TileIndex& tileIndex) const;

    void addTileset(TilesetId tilesetId, Tileset tileset);
    ImageId getTilesetImageId(TilesetId tilesetId) const;

    // should be called when any tile in any layer changes
    void updateAnimatedTileIndices();

private:
    std::unordered_map<TilesetId, Tileset> tilesets;
    std::vector<TileMapLayer> layers;
};

namespace edbr::tilemap
{
TileMap::TileIndex worldPosToTileIndex(const glm::vec2& worldPos);
glm::vec2 tileIndexToWorldPos(const TileMap::TileIndex& tileIndex);
math::FloatRect getTileAABB(const TileMap::TileIndex& tileIndex);
math::IndexRange2 getTileIndicesInRect(const math::FloatRect& rect);

std::pair<glm::vec2, glm::vec2> tileIdToUVs(
    TileMap::TileId tileId,
    const glm::ivec2& tilesetTextureSize);
}
