#pragma once

class TileMap;
class SpriteRenderer;
class GfxDevice;

class TileMapRenderer {
public:
    void drawTileMapLayer(
        GfxDevice& gfxDevice,
        SpriteRenderer& spriteRenderer,
        const TileMap& tileMap,
        int z) const;

private:
};
