#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <vector>

class Renderer {
public:
    static constexpr std::size_t FRAME_OVERLAP = 2;

    struct FrameData {
        VkCommandPool commandPool;
        VkCommandBuffer mainCommandBuffer;

        VkSemaphore swapchainSemaphore;
        VkSemaphore renderSemaphore;
        VkFence renderFence;
    };

public:
    Renderer(VkQueue graphicsQueue, std::uint32_t graphicsQueueFamily);
    void createCommandBuffers(VkDevice device);
    void initSyncStructures(VkDevice device);

    void destroyCommandBuffers(VkDevice device);
    void destroySyncStructures(VkDevice device);

    FrameData& getCurrentFrame();
    void draw(
        VkDevice device,
        VkSwapchainKHR swapchain,
        const std::vector<VkImage>& swapchainImages);

private:
    std::array<FrameData, FRAME_OVERLAP> frames{};
    std::uint32_t frameNumber{0};

    VkQueue graphicsQueue;
    std::uint32_t graphicsQueueFamily;
};
