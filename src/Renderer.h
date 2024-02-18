#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <vector>

#include <DeletionQueue.h>
#include <VkDescriptors.h>
#include <VkTypes.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include <glm/vec4.hpp>

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
    void initDescriptors();
    void initPipelines();
    void initBackgroundPipelines();

    void initImGui();

    void destroyCommandBuffers();
    void destroySyncStructures();

    void update(float dt);

    void imGuiImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& f);

    FrameData& getCurrentFrame();

    void draw();
    void drawBackground(VkCommandBuffer cmd);
    void drawImGui(VkCommandBuffer cmd, VkImageView targetImageView);

    GLFWwindow* window{nullptr};

    vkb::Instance instance;
    vkb::PhysicalDevice physicalDevice;
    vkb::Device device;
    VmaAllocator allocator;

    VkQueue graphicsQueue;
    std::uint32_t graphicsQueueFamily;

    DeletionQueue deletionQueue;

    VkSurfaceKHR surface;

    vkb::Swapchain swapchain;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkExtent2D swapchainExtent;

    AllocatedImage drawImage;
    VkExtent2D drawExtent;

    std::array<FrameData, FRAME_OVERLAP> frames{};
    std::uint32_t frameNumber{0};

    DescriptorAllocator descriptorAllocator;
    VkDescriptorSet drawImageDescriptors;
    VkDescriptorSetLayout drawImageDescriptorLayout;

    VkPipeline gradientPipeline;
    VkPipelineLayout gradientPipelineLayout;

    VkFence imguiFence;
    VkCommandBuffer imguiCommandBuffer;
    VkCommandPool imguiCommandPool;

    struct ComputePushConstants {
        glm::vec4 data1;
        glm::vec4 data2;
        glm::vec4 data3;
        glm::vec4 data4;
    };

    ComputePushConstants gradientConstants;
};
