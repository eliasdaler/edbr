#include <edbr/TileMap/TileMapRenderer.h>

#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/Sprite.h>
#include <edbr/Graphics/SpriteRenderer.h>
#include <edbr/TileMap/TileMap.h>

void TileMapRenderer::drawTileMapLayers(
    GfxDevice& gfxDevice,
    SpriteRenderer& spriteRenderer,
    const TileMap& tileMap,
    int z) const
{
    for (const auto& layer : tileMap.getLayers()) {
        if (layer.z != z || !layer.isVisible) {
            continue;
        }
        if (layer.name == TileMap::CollisionLayerName) {
            continue;
        }
        drawTileMapLayer(gfxDevice, spriteRenderer, tileMap, layer);
    }
}

void TileMapRenderer::drawTileMapLayer(
    GfxDevice& gfxDevice,
    SpriteRenderer& spriteRenderer,
    const TileMap& tileMap,
    const TileMap::TileMapLayer& layer) const
{
    for (const auto& [ti, tile] : layer.tiles) {
        // TODO: don't draw tiles which are not visible on the screen

        Sprite sprite;
        const auto tilesetImageId = tileMap.getTilesetImageId(tile.tilesetId);
        assert(tilesetImageId != NULL_IMAGE_ID);

        const auto& tilesetImage = gfxDevice.getImage(tilesetImageId);
        sprite.setTexture(gfxDevice.getImage(tilesetImageId));
        const auto [uv0, uv1] = edbr::tilemap::tileIdToUVs(tile.id, tilesetImage.getSize2D());
        sprite.uv0 = uv0;
        sprite.uv1 = uv1;

        spriteRenderer.drawSprite(gfxDevice, sprite, edbr::tilemap::tileIndexToWorldPos(ti));
    }
}
