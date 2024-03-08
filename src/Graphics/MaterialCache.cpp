#include "MaterialCache.h"

#include "GfxDevice.h"

void MaterialCache::addMaterial(MaterialId id, Material material)
{
    materials.push_back(std::move(material));
}

const Material& MaterialCache::getMaterial(MaterialId id) const
{
    return materials.at(id);
}

MaterialId MaterialCache::getFreeMaterialId() const
{
    return materials.size();
}

void MaterialCache::cleanup(const GfxDevice& gfxDevice)
{
    for (const auto& material : materials) {
        if (material.hasDiffuseTexture) {
            gfxDevice.destroyImage(material.diffuseTexture);
        }
    }
    materials.clear();
}
