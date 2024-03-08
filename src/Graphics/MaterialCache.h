#pragma once

#include <vector>

#include <Graphics/IdTypes.h>
#include <Graphics/Material.h>

class GfxDevice;

class MaterialCache {
public:
    void addMaterial(MaterialId id, Material material);
    const Material& getMaterial(MaterialId id) const;

    MaterialId getFreeMaterialId() const;

    void cleanup(const GfxDevice& gfxDevice);

private:
    std::vector<Material> materials;
};
