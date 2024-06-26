#pragma once

#include <vector>

#include <edbr/Graphics/GPUMesh.h>

class GfxDevice;
struct CPUMesh;

class MeshCache {
public:
    void cleanup(const GfxDevice& gfxDevice);

    MeshId addMesh(GfxDevice& gfxDevice, const CPUMesh& cpuMesh);
    const GPUMesh& getMesh(MeshId id) const;

private:
    void uploadMesh(GfxDevice& gfxDevice, const CPUMesh& cpuMesh, GPUMesh& gpuMesh) const;

    std::vector<GPUMesh> meshes;
};
