#pragma once

#include <vulkan/vulkan.h>

#include <edbr/Graphics/Vulkan/Util.h>
#include <edbr/Graphics/Vulkan/VulkanImmediateExecutor.h>

#include <edbr/Graphics/MeshCache.h>

struct CPUMesh;
class GfxDevice;

class BaseRenderer {
public:
    BaseRenderer(GfxDevice& gfxDevice);
    BaseRenderer(const BaseRenderer&) = delete;
    BaseRenderer& operator=(const BaseRenderer&) = delete;

    void init();
    void cleanup();

public:
    MeshId addMesh(const CPUMesh& cpuMesh, MaterialId material);
    void uploadMesh(const CPUMesh& cpuMesh, GPUMesh& mesh) const;
    const GPUMesh& getMesh(MeshId id) const;

    GfxDevice& getGfxDevice() { return gfxDevice; }

private:
    void initSamplers();

private: // data
    GfxDevice& gfxDevice;

    MeshCache meshCache;
};
