#pragma once

#include <vector>

#include <Graphics/Material.h>

class MaterialCache {
public:
    MaterialId addMaterial(Material material);

    const Material& getMaterial(MaterialId id) const;

private:
    std::vector<Material> materials;
};
