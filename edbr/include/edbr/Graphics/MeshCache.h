#pragma once

#include <vector>

#include <edbr/Graphics/GPUMesh.h>

class GfxDevice;

class MeshCache {
public:
    MeshId addMesh(GPUMesh mesh);

    const GPUMesh& getMesh(MeshId id) const;

    void cleanup(const GfxDevice& gfxDevice);

private:
    std::vector<GPUMesh> meshes;
};
