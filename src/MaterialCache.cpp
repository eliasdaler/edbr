#include "MaterialCache.h"

MaterialId MaterialCache::addMaterial(Material material)
{
    // TODO: check if all properties of the material are same and return
    // already cached material.
    const auto id = materials.size();
    materials.push_back(std::move(material));
    return id;
}

const Material& MaterialCache::getMaterial(MaterialId id) const
{
    return materials.at(id);
}
