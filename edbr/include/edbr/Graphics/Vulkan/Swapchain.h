#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <vulkan/vulkan.h>

#include <VkBootstrap.h>

#include <edbr/Graphics/Common.h>

class Swapchain {
public:
    void initSyncStructures(VkDevice device);
    void create(
        const vkb::Device& device,
        VkFormat format,
        std::uint32_t width,
        std::uint32_t height,
        bool vSync);
    void recreate(
        const vkb::Device& device,
        VkFormat format,
        std::uint32_t width,
        std::uint32_t height,
        bool vSync);
    void cleanup(VkDevice device);

    VkExtent2D getExtent() const { return swapchain.extent; }

    const std::vector<VkImage>& getImages() { return images; };

    void beginFrame(VkDevice device, std::size_t frameIndex) const;
    void resetFences(VkDevice device, std::size_t frameIndex) const;

    // returns the image and its index
    std::pair<VkImage, std::uint32_t> acquireImage(VkDevice device, std::size_t frameIndex);

    void submitAndPresent(
        VkCommandBuffer cmd,
        VkQueue graphicsQueue,
        std::size_t frameIndex,
        std::uint32_t swapchainImageIndex);

    VkImageView getImageView(std::size_t swapchainImageIndex)
    {
        return imageViews[swapchainImageIndex];
    }

    bool needsRecreation() const { return dirty; }

private:
    struct FrameData {
        VkSemaphore swapchainSemaphore;
        VkSemaphore renderSemaphore;
        VkFence renderFence;
    };

    std::array<FrameData, graphics::FRAME_OVERLAP> frames;
    vkb::Swapchain swapchain;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    bool dirty{false};
};
