#pragma once

#include <vector>

#include <edbr/Graphics/IdTypes.h>
#include <edbr/Graphics/Material.h>

class MaterialCache {
    friend class ResourcesInspector;

public:
    void addMaterial(MaterialId id, Material material);
    const Material& getMaterial(MaterialId id) const;

    MaterialId getFreeMaterialId() const;

private:
    std::vector<Material> materials;
};
