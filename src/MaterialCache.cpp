#include "MaterialCache.h"

#include "Renderer.h"

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

void MaterialCache::cleanup(const Renderer& renderer)
{
    for (const auto& material : materials) {
        if (material.hasDiffuseTexture) {
            renderer.destroyImage(material.diffuseTexture);
        }
    }
    materials.clear();
}
