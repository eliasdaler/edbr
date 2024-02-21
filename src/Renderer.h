#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include <DeletionQueue.h>
#include <VkDescriptors.h>
#include <VkTypes.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include <glm/vec4.hpp>

#include "MaterialCache.h"
#include "MeshCache.h"

#include "FreeCameraController.h"

#include <Graphics/Camera.h>
#include <Graphics/Mesh.h>

class SDL_Window;

struct Scene;
struct SceneNode;

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

        DescriptorAllocatorGrowable frameDescriptors;
    };

    static const std::size_t NULL_ENTITY_ID = std::numeric_limits<std::size_t>::max();

    using EntityId = std::size_t;

    struct Entity {
        EntityId id{NULL_ENTITY_ID};
        std::string tag;

        // transform
        Transform transform; // local (relative to parent)
        glm::mat4 worldTransform{1.f};

        // hierarchy
        EntityId parentId{NULL_ENTITY_ID};
        std::vector<EntityId> children;

        // mesh (only one mesh per entity supported for now)
        std::vector<MeshId> meshes;

        // skeleton
        // Skeleton skeleton;
        // bool hasSkeleton{false};

        // animation
        // SkeletonAnimator skeletonAnimator;
        // std::unordered_map<std::string, SkeletalAnimation> animations;

        /* void uploadJointMatricesToGPU(
            const wgpu::Queue& queue,
            const std::vector<glm::mat4>& jointMatrices) const; */
    };

    struct DrawCommand {
        const GPUMesh& mesh;
        std::size_t meshId;
        glm::mat4 transformMatrix;
        math::Sphere worldBoundingSphere;
    };

public:
    void init();
    void run();
    void cleanup();

    [[nodiscard]] GPUMeshBuffers uploadMesh(
        std::span<const std::uint32_t> indices,
        std::span<const Mesh::Vertex> vertices) const;

    [[nodiscard]] AllocatedBuffer createBuffer(
        std::size_t allocSize,
        VkBufferUsageFlags usage,
        VmaMemoryUsage memoryUsage) const;

    [[nodiscard]] AllocatedImage createImage(
        void* data,
        VkExtent3D size,
        VkFormat format,
        VkImageUsageFlags usage,
        bool mipMap);

    void destroyBuffer(const AllocatedBuffer& buffer) const;
    void destroyImage(const AllocatedImage& image) const;

    VkDescriptorSet writeMaterialData(MaterialId id, const Material& material);

private:
    void initVulkan();
    void createSwapchain(std::uint32_t width, std::uint32_t height);
    void createCommandBuffers();
    void initSyncStructures();
    void initImmediateStructures();
    void initDescriptors();

    void allocateMaterialDataBuffer(std::size_t numMaterials);

    void initPipelines();
    void initBackgroundPipelines();
    void initTrianglePipeline();
    void initMeshPipeline();

    void initImGui();

    void destroyCommandBuffers();
    void destroySyncStructures();

    AllocatedImage createImage(
        VkExtent3D size,
        VkFormat format,
        VkImageUsageFlags usage,
        bool mipMap);

    void handleInput(float dt);
    void update(float dt);

    void updateDevTools(float dt);

    FrameData& getCurrentFrame();

    void draw();
    void drawBackground(VkCommandBuffer cmd);
    void drawGeometry(VkCommandBuffer cmd);
    void drawImGui(VkCommandBuffer cmd, VkImageView targetImageView);

    void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const;

    SDL_Window* window{nullptr};

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
    AllocatedImage depthImage;
    VkExtent2D drawExtent;

    std::array<FrameData, FRAME_OVERLAP> frames{};
    std::uint32_t frameNumber{0};

    DescriptorAllocatorGrowable descriptorAllocator;
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

    VkDescriptorSetLayout sceneDataDescriptorLayout;

    VkPipelineLayout meshPipelineLayout;
    VkPipeline meshPipeline;
    VkDescriptorSetLayout meshMaterialLayout;

    MaterialCache materialCache;
    MeshCache meshCache;

    bool isRunning{false};
    bool vSync{true};
    bool frameLimit{true};
    float frameTime{0.f};
    float avgFPS{0.f};

    // only display update FPS every 1 seconds, otherwise it's too noisy
    float displayedFPS{0.f};
    float displayFPSDelay{1.f};

    Camera camera;
    FreeCameraController cameraController;

    AllocatedImage whiteTexture;
    VkSampler defaultSamplerNearest;

    std::vector<DrawCommand> drawCommands;
    std::vector<std::size_t> sortedDrawCommands;

    void createEntitiesFromScene(const Scene& scene);
    EntityId createEntitiesFromNode(
        const Scene& scene,
        const SceneNode& node,
        EntityId parentId = NULL_ENTITY_ID);

    std::vector<std::unique_ptr<Entity>> entities;
    Entity& makeNewEntity();
    Entity& findEntityByName(std::string_view name) const;

    void updateEntityTransforms();
    void updateEntityTransforms(Entity& e, const glm::mat4& parentWorldTransform);

    void generateDrawList();
    void sortDrawList();

    glm::vec4 ambientColorAndIntensity;
    glm::vec4 sunlightDir;
    glm::vec4 sunlightColorAndIntensity;

    AllocatedBuffer materialDataBuffer;
};
