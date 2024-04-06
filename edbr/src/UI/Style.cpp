#include <edbr/Graphics/GfxDevice.h>
#include <edbr/UI/Style.h>

namespace ui
{
void NineSliceStyle::load(const JsonDataLoader& loader, GfxDevice& gfxDevice)
{
    // load texture
    loader.get("texture", textureFilename);
    texture = gfxDevice.loadImageFromFile(textureFilename);
    assert(texture != NULL_IMAGE_ID);

    if (loader.hasKey("textureCoords")) {
        const auto cl = loader.getLoader("textureCoords");
        cl.get("corner", coords.corner);
        cl.get("horizontal", coords.horizonal);
        cl.get("vertical", coords.vertical);
        cl.get("contents", coords.contents);
    }
}

}
