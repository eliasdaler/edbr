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

#include <glm/vec4.hpp>

#include <edbr/Graphics/Color.h>
#include <edbr/Graphics/Common.h>
#include <edbr/Graphics/ImageCache.h>
#include <edbr/Graphics/Vulkan/Swapchain.h>
#include <edbr/Graphics/Vulkan/VulkanImGuiBackend.h>
#include <edbr/Graphics/Vulkan/VulkanImmediateExecutor.h>
#include <edbr/Version.h>

namespace vkutil
{
struct CreateImageInfo;
}

struct GPUBuffer;
struct GPUImage;

struct SDL_Window;

class GfxDevice {
public:
    struct FrameData {
        VkCommandPool commandPool;
        VkCommandBuffer mainCommandBuffer;
        TracyVkCtx tracyVkCtx;
    };

public:
    GfxDevice();
    GfxDevice(const GfxDevice&) = delete;
    GfxDevice& operator=(const GfxDevice&) = delete;

    void init(SDL_Window* window, const char* appName, const Version& appVersion, bool vSync);
    void recreateSwapchain(std::uint32_t swapchainWidth, std::uint32_t swapchainHeight);

    VkCommandBuffer beginFrame();

    struct EndFrameProps {
        const LinearColor clearColor{0.f, 0.f, 0.f, 1.f};
        bool copyImageIntoSwapchain{true};
        glm::ivec4 drawImageBlitRect{}; // where to blit draw image to
        bool drawImageLinearBlit{true}; // if false - nearest filter will be used
        bool drawImGui{true};
    };
    void endFrame(VkCommandBuffer cmd, const GPUImage& drawImage, const EndFrameProps& props);
    void cleanup();

    [[nodiscard]] GPUBuffer createBuffer(
        std::size_t allocSize,
        VkBufferUsageFlags usage,
        VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO) const;
    [[nodiscard]] VkDeviceAddress getBufferAddress(const GPUBuffer& buffer) const;
    void destroyBuffer(const GPUBuffer& buffer) const;

    bool deviceSupportsSamplingCount(VkSampleCountFlagBits sample) const;
    VkSampleCountFlagBits getMaxSupportedSamplingCount() const;
    float getMaxAnisotropy() const { return maxSamplerAnisotropy; }

    VulkanImmediateExecutor createImmediateExecutor() const;
    void immediateSubmit(std::function<void(VkCommandBuffer)>&& f) const;

    void waitIdle() const;

    BindlessSetManager& getBindlessSetManager();
    VkDescriptorSetLayout getBindlessDescSetLayout() const;
    const VkDescriptorSet& getBindlessDescSet() const;
    void bindBindlessDescSet(VkCommandBuffer cmd, VkPipelineLayout layout) const;

public:
    [[nodiscard]] ImageId createImage(
        const vkutil::CreateImageInfo& createInfo,
        const char* debugName = nullptr,
        void* pixelData = nullptr,
        ImageId imageId = NULL_IMAGE_ID);

    // create a color image which can be used as a draw target
    [[nodiscard]] ImageId createDrawImage(VkFormat format, glm::ivec2 size, const char* debugName);

    [[nodiscard]] ImageId loadImageFromFile(
        const std::filesystem::path& path,
        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
        VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        bool mipMap = false);

    ImageId addImageToCache(GPUImage image);

    [[nodiscard]] const GPUImage& getImage(ImageId id) const;
    void uploadImageData(const GPUImage& image, void* pixelData, std::uint32_t layer = 0) const;

    ImageId getWhiteTextureID() { return whiteImageId; }

    // createImageRaw is mostly intended for low level usage. In most cases,
    // createImage should be preferred as it will automatically
    // add the image to bindless set
    [[nodiscard]] GPUImage createImageRaw(const vkutil::CreateImageInfo& createInfo) const;
    // loadImageFromFileRaw is mostly intended for low level usage. In most cases,
    // loadImageFromFile should be preferred as it will automatically
    // add the image to bindless set
    [[nodiscard]] GPUImage loadImageFromFileRaw(
        const std::filesystem::path& path,
        VkFormat format,
        VkImageUsageFlags usage,
        bool mipMap) const;
    // destroyImage should only be called on images not beloning to image cache / bindless set
    void destroyImage(const GPUImage& image) const;

    // for dev tools only - don't use directly
    const ImageCache& getImageCache() const { return imageCache; }

public:
    VkDevice getDevice() const { return device; }

    std::uint32_t getCurrentFrameIndex() const;

    VkExtent2D getSwapchainExtent() const { return swapchain.getExtent(); }
    glm::ivec2 getSwapchainSize() const
    {
        return {getSwapchainExtent().width, getSwapchainExtent().height};
    }

    VkFormat getSwapchainFormat() const { return swapchainFormat; }
    const TracyVkCtx& getTracyVkCtx() const;

    bool needsSwapchainRecreate() const { return swapchain.needsRecreation(); }

private:
    void initVulkan(SDL_Window* window, const char* appName, const Version& appVersion);
    void checkDeviceCapabilities();
    void createCommandBuffers();

    FrameData& getCurrentFrame();

private: // data
    vkb::Instance instance;
    vkb::PhysicalDevice physicalDevice;
    vkb::Device device;
    VmaAllocator allocator;

    std::uint32_t graphicsQueueFamily;
    VkQueue graphicsQueue;

    VkSurfaceKHR surface;
    VkFormat swapchainFormat;
    Swapchain swapchain;

    std::array<FrameData, graphics::FRAME_OVERLAP> frames{};
    std::uint32_t frameNumber{0};

    VulkanImmediateExecutor executor;

    VulkanImGuiBackend imGuiBackend;

    VkSampleCountFlagBits supportedSampleCounts;
    VkSampleCountFlagBits highestSupportedSamples{VK_SAMPLE_COUNT_1_BIT};
    float maxSamplerAnisotropy{1.f};

    ImageCache imageCache;

    ImageId whiteImageId{NULL_IMAGE_ID};
    ImageId errorImageId{NULL_IMAGE_ID};

    bool vSync{true};
};
