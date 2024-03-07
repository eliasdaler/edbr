#pragma once

#include <array>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkFormat format;
    VkImageUsageFlags usage;
    VkExtent3D extent;
    std::uint32_t mipLevels{1};
    std::uint32_t numLayers{1};
    bool isCubemap{false};

    VkExtent2D getExtent2D() const { return VkExtent2D{extent.width, extent.height}; }
};

struct AllocatedBuffer {
    VkBuffer buffer{VK_NULL_HANDLE};
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct GPUMeshBuffers {
    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress{};
};

struct GPUDrawPushConstants {
    glm::mat4 transform;
    VkDeviceAddress vertexBuffer;
};

struct DepthOnlyPushConstants {
    glm::mat4 mvp;
    VkDeviceAddress vertexBuffer;
};
