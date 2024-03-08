#pragma once

#include <vulkan/vulkan.h>

#include <Graphics/MaterialCache.h>
#include <Graphics/MeshCache.h>
#include <Graphics/Vulkan/VulkanImmediateExecutor.h>

struct CPUMesh;

class BaseRenderer {
public:
    BaseRenderer(Renderer& renderer);

    void init();
    void cleanup();

public:
    VkSampler getDefaultNearestSampler() const { return defaultNearestSampler; }
    VkSampler getDefaultLinearSampler() const { return defaultLinearSampler; }
    VkSampler getDefaultShadowMapSampler() const { return defaultShadowMapSampler; }

    VkDescriptorSetLayout getMaterialDataDescSetLayout() const { return meshMaterialLayout; }

    MeshId addMesh(const CPUMesh& cpuMesh, MaterialId material);
    void uploadMesh(const CPUMesh& cpuMesh, GPUMesh& mesh) const;
    SkinnedMesh createSkinnedMeshBuffer(MeshId meshId) const;
    const GPUMesh& getMesh(MeshId id) const;

    MaterialId addMaterial(Material material);
    const Material& getMaterial(MaterialId id) const;

    Renderer& getRenderer() { return renderer; }

private:
    void initSamplers();
    void initDefaultTextures();
    void allocateMaterialDataBuffer();
    void initDescriptors();

private: // data
    Renderer& renderer;
    VulkanImmediateExecutor executor;

    MeshCache meshCache;

    static const auto MAX_MATERIALS = 1000;
    AllocatedBuffer materialDataBuffer;

    MaterialCache materialCache;
    VkDescriptorSetLayout meshMaterialLayout;

    VkSampler defaultNearestSampler;
    VkSampler defaultLinearSampler;
    VkSampler defaultShadowMapSampler;

    AllocatedImage whiteTexture;
};
