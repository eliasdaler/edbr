#pragma once

#include <vulkan/vulkan.h>

#include <edbr/Graphics/Vulkan/Util.h>
#include <edbr/Graphics/Vulkan/VulkanImmediateExecutor.h>

#include <edbr/Graphics/MaterialCache.h>
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
    VkSampler getNearestSampler() const { return nearestSampler; }
    VkSampler getLinearSampler() const { return linearSampler; }
    VkSampler getShadowMapSampler() const { return shadowMapSampler; }

    MeshId addMesh(const CPUMesh& cpuMesh, MaterialId material);
    void uploadMesh(const CPUMesh& cpuMesh, GPUMesh& mesh) const;
    [[nodiscard]] SkinnedMesh createSkinnedMeshBuffer(MeshId meshId) const;
    const GPUMesh& getMesh(MeshId id) const;

    MaterialId addMaterial(Material material);
    const Material& getMaterial(MaterialId id) const;

    // for use in ImGui tools
    const MaterialCache& getMaterialCache() const { return materialCache; }

    GfxDevice& getGfxDevice() { return gfxDevice; }

    VkDeviceAddress getMaterialDataBufferAddress() const { return materialDataBuffer.address; }

private:
    void initSamplers();
    void initDefaultTextures();
    void allocateMaterialDataBuffer();

private: // data
    GfxDevice& gfxDevice;
    VulkanImmediateExecutor executor;

    MeshCache meshCache;
    MaterialCache materialCache;

    static const auto MAX_MATERIALS = 1000;
    GPUBuffer materialDataBuffer;

    VkSampler nearestSampler;
    VkSampler linearSampler;
    VkSampler shadowMapSampler;

    ImageId placeholderNormalMapTextureID;
};
