#pragma once

#include <vector>

#include <Graphics/GPUMesh.h>

class Renderer;

class MeshCache {
public:
    MeshId addMesh(GPUMesh mesh);

    const GPUMesh& getMesh(MeshId id) const;

    void cleanup(const Renderer& renderer);

private:
    std::vector<GPUMesh> meshes;
};
