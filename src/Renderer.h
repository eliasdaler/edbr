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

#include "MaterialCache.h"
#include "MeshCache.h"

#include <Graphics/Mesh.h>
#include <Graphics/Scene.h>

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

    GPUMeshBuffers uploadMesh(
        std::span<const std::uint32_t> indices,
        std::span<const Mesh::Vertex> vertices) const;
    void destroyBuffer(const AllocatedBuffer& buffer) const;

private:
    void initVulkan();
    void createSwapchain(std::uint32_t width, std::uint32_t height);
    void createCommandBuffers();
    void initSyncStructures();
    void initImmediateStructures();
    void initDescriptors();

    void initPipelines();
    void initBackgroundPipelines();
    void initTrianglePipeline();
    void initMeshPipeline();

    void initImGui();

    void destroyCommandBuffers();
    void destroySyncStructures();

    AllocatedBuffer createBuffer(
        std::size_t allocSize,
        VkBufferUsageFlags usage,
        VmaMemoryUsage memoryUsage) const;

    void update(float dt);

    FrameData& getCurrentFrame();

    void draw();
    void drawBackground(VkCommandBuffer cmd);
    void drawGeometry(VkCommandBuffer cmd);
    void drawImGui(VkCommandBuffer cmd, VkImageView targetImageView);

    void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const;

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

    VkFence immFence;
    VkCommandBuffer immCommandBuffer;
    VkCommandPool immCommandPool;

    struct ComputePushConstants {
        glm::vec4 data1;
        glm::vec4 data2;
        glm::vec4 data3;
        glm::vec4 data4;
    };

    ComputePushConstants gradientConstants;

    VkPipelineLayout trianglePipelineLayout;
    VkPipeline trianglePipeline;

    VkPipelineLayout meshPipelineLayout;
    VkPipeline meshPipeline;

    MaterialCache materialCache;
    MeshCache meshCache;

    Scene scene;
};
