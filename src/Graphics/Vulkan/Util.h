#pragma once

#include <cassert>
#include <cstdint>
#include <filesystem>

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <glm/vec4.hpp>

#define VK_CHECK(call)                 \
    do {                               \
        VkResult result_ = call;       \
        assert(result_ == VK_SUCCESS); \
    } while (0)

struct AllocatedImage;
struct AllocatedBuffer;
struct VulkanImmediateExecutor;

namespace vkutil
{

struct CreateImageInfo {
    VkFormat format;
    VkImageUsageFlags usage;
    VkExtent3D extent{};
    std::uint32_t numLayers{1};
    bool mipMap{false};
};

[[nodiscard]] AllocatedImage createImage(
    VkDevice device,
    VmaAllocator allocator,
    const CreateImageInfo& createInfo);

void destroyImage(VkDevice device, VmaAllocator allocator, const AllocatedImage& image);

void uploadImageData(
    VkDevice device,
    std::uint32_t graphicsQueueFamily,
    VkQueue graphicsQueue,
    VmaAllocator allocator,
    const AllocatedImage& image,
    void* pixelData);

[[nodiscard]] AllocatedImage loadImageFromFile(
    VkDevice device,
    std::uint32_t graphicsQueueFamily,
    VkQueue graphicsQueue,
    VmaAllocator allocator,
    const std::filesystem::path& path,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipMap);

void transitionImage(
    VkCommandBuffer cmd,
    VkImage image,
    VkImageLayout currentLayout,
    VkImageLayout newLayout);

void copyImageToImage(
    VkCommandBuffer cmd,
    VkImage source,
    VkImage destination,
    VkExtent2D srcSize,
    VkExtent2D dstSize);

void generateMipmaps(
    VkCommandBuffer cmd,
    VkImage image,
    VkExtent2D imageSize,
    std::uint32_t mipLevels);

AllocatedBuffer createBuffer(
    VmaAllocator allocator,
    std::size_t allocSize,
    VkBufferUsageFlags usage,
    VmaMemoryUsage memoryUsage);
void destroyBuffer(VmaAllocator allocator, const AllocatedBuffer& buffer);

void cmdBeginLabel(VkCommandBuffer cmd, const char* label, glm::vec4 color = glm::vec4{1.f});
void cmdEndLabel(VkCommandBuffer cmd);

void addDebugLabel(VkDevice device, VkImage image, const char* label);
void addDebugLabel(VkDevice device, VkImageView imageView, const char* label);
void addDebugLabel(VkDevice device, VkShaderModule shaderModule, const char* label);
void addDebugLabel(VkDevice device, VkPipeline pipeline, const char* label);
void addDebugLabel(VkDevice device, VkBuffer buffer, const char* label);

} // end of namespace vkutil
