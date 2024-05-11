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
        std::string name;
        int z{false};
        bool isVisible{true};
        std::unordered_map<TileIndex, Tile, math::hash<TileIndex>> tiles;

        // returns a tile with NULL_TILE_ID and NULL_TILESET_ID if tile is not
        // present on a layer
        const Tile& getTile(const TileIndex& tileIndex) const;

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
        std::string name;
        ImageId texture;
        glm::ivec2 textureSize;

        std::unordered_map<TileId, TilesetAnimation> animations; // first tile id -> animation
        std::unordered_map<TileId, SpriteAnimator> tileAnimators;
    };

    static constexpr auto CollisionLayerName = "Collision";

    void load(const JsonDataLoader& loader, GfxDevice& gfxDevice);
    void clear();

    void update(float dt);

    int getMinLayerZ() const;
    int getMaxLayerZ() const;
    bool layerExists(int z) const;

    static TileIndex GetTileIndex(const glm::vec2& worldPos);
    static math::FloatRect GetTileAABB(const TileIndex& tileIndex);

    TileMap::TileMapLayer& addLayer(TileMapLayer layer);
    const TileMap::TileMapLayer& getLayer(const std::string& name) const;
    const std::vector<TileMap::TileMapLayer>& getLayers() const { return layers; }

    // Returns a tile with NULL_TILE_ID and NULL_TILESET_ID if tile is not
    // present on a layer
    const Tile& getTile(const std::string& layerName, const TileIndex& tileIndex) const;

    math::IndexRange2 getTileIndicesInRect(const math::FloatRect& rect) const;

    void addTileset(TilesetId tilesetId, Tileset tileset);
    ImageId getTilesetImageId(TilesetId tilesetId) const;

    // should be called when tile layers change
    void updateAnimatedTileIndices();

private:
    std::vector<TileMapLayer> layers;

    std::unordered_map<TilesetId, Tileset> tilesets;
};

namespace edbr::tilemap
{
glm::vec2 tileIndexToWorldPos(const TileMap::TileIndex& tileIndex);
std::pair<glm::vec2, glm::vec2> tileIdToUVs(
    TileMap::TileId tileId,
    const glm::ivec2& tilesetTextureSize);
}
