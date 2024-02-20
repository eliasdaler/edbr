#pragma once

#include <vector>

#include <Graphics/Material.h>

class Renderer;

class MaterialCache {
public:
    MaterialId addMaterial(Material material);

    const Material& getMaterial(MaterialId id) const;

    void cleanup(const Renderer& renderer);

private:
    std::vector<Material> materials;
};
