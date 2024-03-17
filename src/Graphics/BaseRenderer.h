#pragma once

#include <filesystem>

#include <vulkan/vulkan.h>

#include <Graphics/Vulkan/Util.h>
#include <Graphics/Vulkan/VulkanImmediateExecutor.h>

#include <Graphics/ImageCache.h>
#include <Graphics/MaterialCache.h>
#include <Graphics/MeshCache.h>

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

    GfxDevice& getGfxDevice() { return gfxDevice; }

    [[nodiscard]] ImageId createImage(
        const vkutil::CreateImageInfo& createInfo,
        const char* debugName = nullptr,
        void* pixelData = nullptr);
    [[nodiscard]] ImageId loadImageFromFile(
        const std::filesystem::path& path,
        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
        VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        bool mipMap = false);
    [[nodiscard]] ImageId loadCubemap(const std::filesystem::path& dirPath);

    [[nodiscard]] const AllocatedImage& getImage(ImageId id) const;
    ImageId addImageToCache(AllocatedImage image);

    static const auto MAX_MATERIALS = 1000;
    AllocatedBuffer materialDataBuffer;

    VkDescriptorSetLayout getBindlessDescSetLayout() const;
    VkDescriptorSet getBindlessDescSet() const;

    ImageId getWhiteTextureID() { return whiteTextureID; }

private:
    void initSamplers();
    void initDefaultTextures();
    void allocateMaterialDataBuffer();

private: // data
    GfxDevice& gfxDevice;
    VulkanImmediateExecutor executor;

    MeshCache meshCache;
    MaterialCache materialCache;

    ImageCache imageCache;

    VkSampler nearestSampler;
    VkSampler linearSampler;
    VkSampler shadowMapSampler;

    ImageId whiteTextureID;
    ImageId placeholderNormalMapTextureID;
};
