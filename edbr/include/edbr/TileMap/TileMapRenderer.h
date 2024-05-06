#pragma once

#include <edbr/TileMap/TileMap.h>

class SpriteRenderer;
class GfxDevice;

class TileMapRenderer {
public:
    void drawTileMapLayers(
        GfxDevice& gfxDevice,
        SpriteRenderer& spriteRenderer,
        const TileMap& tileMap,
        int z) const;

    void drawTileMapLayer(
        GfxDevice& gfxDevice,
        SpriteRenderer& spriteRenderer,
        const TileMap& tileMap,
        const TileMap::TileMapLayer& layer) const;

private:
};
