#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <vector>

#include <DeletionQueue.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

class GLFWwindow;

class Renderer {
public:
    static constexpr std::size_t FRAME_OVERLAP = 2;

    struct FrameData {
        VkCommandPool commandPool;
        VkCommandBuffer mainCommandBuffer;

        VkSemaphore swapchainSemaphore;
        VkSemaphore renderSemaphore;
        VkFence renderFence;

        DeletionQueue deletionQueue;
    };

public:
    void init();
    void run();
    void cleanup();

private:
    void initVulkan();
    void createSwapchain(std::uint32_t width, std::uint32_t height);
    void createCommandBuffers();
    void initSyncStructures();

    void destroyCommandBuffers();
    void destroySyncStructures();

    FrameData& getCurrentFrame();
    void draw();

    GLFWwindow* window{nullptr};

    vkb::Instance instance;
    vkb::Device device;

    vkb::Swapchain swapchain;
    VkSurfaceKHR surface;
    std::vector<VkImage> swapchainImages;

    VmaAllocator allocator;
    VkQueue graphicsQueue;
    std::uint32_t graphicsQueueFamily;

    DeletionQueue deletionQueue;

    std::array<FrameData, FRAME_OVERLAP> frames{};
    std::uint32_t frameNumber{0};
};
