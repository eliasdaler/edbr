#pragma once

#include <array>
#include <cstdint>
#include <filesystem>

// don't sort these includes
// clang-format off
#include <vulkan/vulkan.h>
#include <volk.h> // include needed for TracyVulkan.hpp
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <tracy/TracyVulkan.hpp>
// clang-format on

#include <Graphics/Common.h>
#include <Graphics/Swapchain.h>
#include <Graphics/Vulkan/Descriptors.h>
#include <Graphics/Vulkan/Types.h>
#include <Graphics/Vulkan/VulkanImGui.h>
#include <Graphics/Vulkan/VulkanImmediateExecutor.h>

namespace vkutil
{
struct CreateImageInfo;
}

class GfxDevice {
public:
    struct FrameData {
        VkCommandPool commandPool;
        VkCommandBuffer mainCommandBuffer;
        TracyVkCtx tracyVkCtx;
    };

public:
    GfxDevice() = default;
    GfxDevice(const GfxDevice&) = delete;
    GfxDevice& operator=(const GfxDevice&) = delete;

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

    bool deviceSupportsSamplingCount(VkSampleCountFlagBits sample) const;
    VkSampleCountFlagBits getMaxSupportedSamplingCount() const;
    float getMaxAnisotropy() const { return maxSamplerAnisotropy; }

    VulkanImmediateExecutor createImmediateExecutor() const;

public:
    VkDevice getDevice() const { return device; }
    VmaAllocator getAllocator() const { return allocator; }

    std::uint32_t getCurrentFrameIndex() const;
    VkExtent2D getSwapchainExtent() const { return swapchain.getExtent(); }
    const TracyVkCtx& getTracyVkCtx() const;

    VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout layout);

private:
    void initVulkan(SDL_Window* window);
    void checkDeviceCapabilities();
    void createCommandBuffers();
    void initDescriptorAllocator();

    FrameData& getCurrentFrame();

private: // data
    vkb::Instance instance;
    vkb::PhysicalDevice physicalDevice;
    vkb::Device device;
    VmaAllocator allocator;

    std::uint32_t graphicsQueueFamily;
    VkQueue graphicsQueue;

    VkSurfaceKHR surface;
    Swapchain swapchain;

    DescriptorAllocatorGrowable descriptorAllocator;

    std::array<FrameData, graphics::FRAME_OVERLAP> frames{};
    std::uint32_t frameNumber{0};

    VulkanImmediateExecutor executor;

    VulkanImGuiData imguiData;
    bool imguiDrawn{true};

    VkSampleCountFlagBits supportedSampleCounts;
    VkSampleCountFlagBits highestSupportedSamples{VK_SAMPLE_COUNT_1_BIT};
    float maxSamplerAnisotropy{1.f};
};
