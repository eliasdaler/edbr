#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

// don't sort these includes
// clang-format off
#include <vulkan/vulkan.h>
#include <volk.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <tracy/TracyVulkan.hpp>
// clang-format on

#include <Graphics/MaterialCache.h>
#include <Graphics/MeshCache.h>

#include <Graphics/Vulkan/DeletionQueue.h>
#include <Graphics/Vulkan/Descriptors.h>
#include <Graphics/Vulkan/Types.h>
#include <Graphics/Vulkan/VulkanImGui.h>
#include <Graphics/Vulkan/VulkanImmediateExecutor.h>

struct CPUMesh;

namespace vkutil
{
struct CreateImageInfo;
}

class Renderer {
public:
    static constexpr std::size_t FRAME_OVERLAP = 2;

    struct FrameData {
        VkCommandPool commandPool;
        VkCommandBuffer mainCommandBuffer;

        VkSemaphore swapchainSemaphore;
        VkSemaphore renderSemaphore;
        VkFence renderFence;

        TracyVkCtx tracyVkCtx;
    };

public:
    void init(SDL_Window* window, bool vSync);

    VkCommandBuffer beginFrame();
    void endFrame(VkCommandBuffer cmd, const AllocatedImage& drawImage);
    void cleanup();

    [[nodiscard]] AllocatedBuffer createBuffer(
        std::size_t allocSize,
        VkBufferUsageFlags usage,
        VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO) const;
    [[nodiscard]] VkDeviceAddress getBufferAddress(const AllocatedBuffer& buffer) const;
    void destroyBuffer(const AllocatedBuffer& buffer) const;

    [[nodiscard]] AllocatedImage createImage(const vkutil::CreateImageInfo& createInfo) const;
    void uploadImageData(const AllocatedImage& image, void* pixelData, std::uint32_t layer = 0)
        const;

    [[nodiscard]] AllocatedImage loadImageFromFile(
        const std::filesystem::path& path,
        VkFormat format,
        VkImageUsageFlags usage,
        bool mipMap) const;
    void destroyImage(const AllocatedImage& image) const;

    VkSampler getDefaultNearestSampler() const { return defaultNearestSampler; }
    VkSampler getDefaultLinearSampler() const { return defaultLinearSampler; }
    VkSampler getDefaultShadowMapSample() const { return defaultShadowMapSampler; }

    bool deviceSupportsSamplingCount(VkSampleCountFlagBits sample) const;
    VkSampleCountFlagBits getHighestSupportedSamplingCount() const;

public:
    VkDevice getDevice() const { return device; }
    VmaAllocator getAllocator() const { return allocator; }

    FrameData& getCurrentFrame();
    std::uint32_t getCurrentFrameIndex() const;
    VkExtent2D getSwapchainExtent() const { return swapchainExtent; }

    VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout layout);

    VkDescriptorSetLayout getMaterialDataDescSetLayout() const { return meshMaterialLayout; }

    MeshId addMesh(const CPUMesh& cpuMesh, MaterialId material);
    void uploadMesh(const CPUMesh& cpuMesh, GPUMesh& mesh) const;
    SkinnedMesh createSkinnedMeshBuffer(MeshId meshId) const;
    const GPUMesh& getMesh(MeshId id) const;

    MaterialId addMaterial(Material material);
    const Material& getMaterial(MaterialId id) const;

private:
    void initVulkan(SDL_Window* window);
    void checkDeviceCapabilities();
    void createSwapchain(std::uint32_t width, std::uint32_t height, bool vSync);
    void createCommandBuffers();
    void initSyncStructures();
    void initDescriptorAllocator();
    void initDescriptors();
    void initSamplers();
    void initDefaultTextures();

private: // data
    vkb::Instance instance;
    vkb::PhysicalDevice physicalDevice;
    vkb::Device device;
    VmaAllocator allocator;

    std::uint32_t graphicsQueueFamily;
    VkQueue graphicsQueue;

    VkSurfaceKHR surface;

    vkb::Swapchain swapchain;
    VkExtent2D swapchainExtent;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    DeletionQueue deletionQueue;
    DescriptorAllocatorGrowable descriptorAllocator;

    std::array<FrameData, FRAME_OVERLAP> frames{};
    std::uint32_t frameNumber{0};

    VulkanImmediateExecutor executor;
    VulkanImGuiData imguiData;

    MeshCache meshCache;

    void allocateMaterialDataBuffer();
    static const auto MAX_MATERIALS = 1000;
    AllocatedBuffer materialDataBuffer;

    MaterialCache materialCache;
    VkDescriptorSetLayout meshMaterialLayout;

    VkSampler defaultNearestSampler;
    VkSampler defaultLinearSampler;
    VkSampler defaultShadowMapSampler;
    AllocatedImage whiteTexture;

    VkSampleCountFlagBits supportedSampleCounts;
    VkSampleCountFlagBits highestSupportedSamples{VK_SAMPLE_COUNT_1_BIT};
    float maxSamplerAnisotropy{1.f};

    bool imguiDrawn{true};
};
