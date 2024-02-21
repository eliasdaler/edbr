#pragma once

#include <vector>

#include <Graphics/Material.h>

class Renderer;

class MaterialCache {
public:
    void addMaterial(MaterialId id, Material material);

    const Material& getMaterial(MaterialId id) const;

    void cleanup(const Renderer& renderer);

    MaterialId getFreeMaterialId() const;

private:
    std::vector<Material> materials;
};
