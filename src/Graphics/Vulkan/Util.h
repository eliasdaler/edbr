#pragma once

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <optional>

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
class VulkanImmediateExecutor;

namespace vkutil
{

struct CreateImageInfo {
    VkFormat format;
    VkImageUsageFlags usage;
    VkImageCreateFlags flags;
    VkExtent3D extent{};
    std::uint32_t numLayers{1};
    VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};
    bool mipMap{false};
    bool isCubemap{false};
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
    void* pixelData,
    std::uint32_t layer = 0);

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
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO);
void destroyBuffer(VmaAllocator allocator, const AllocatedBuffer& buffer);

void cmdBeginLabel(VkCommandBuffer cmd, const char* label, glm::vec4 color = glm::vec4{1.f});
void cmdEndLabel(VkCommandBuffer cmd);

void addDebugLabel(VkDevice device, VkImage image, const char* label);
void addDebugLabel(VkDevice device, VkImageView imageView, const char* label);
void addDebugLabel(VkDevice device, VkShaderModule shaderModule, const char* label);
void addDebugLabel(VkDevice device, VkPipeline pipeline, const char* label);
void addDebugLabel(VkDevice device, VkPipelineLayout layout, const char* label);
void addDebugLabel(VkDevice device, VkBuffer buffer, const char* label);

struct RenderingInfoParams {
    VkExtent2D renderExtent;
    VkImageView colorImageView{VK_NULL_HANDLE};
    std::optional<glm::vec4> colorImageClearValue;
    VkImageView depthImageView{VK_NULL_HANDLE};
    std::optional<float> depthImageClearValue;
    VkImageView resolveImageView{VK_NULL_HANDLE};
};

struct RenderInfo {
    VkRenderingAttachmentInfo colorAttachment;
    VkRenderingAttachmentInfo depthAttachment;
    VkRenderingInfo renderingInfo;
};

RenderInfo createRenderingInfo(const RenderingInfoParams& params);

int sampleCountToInt(VkSampleCountFlagBits count);
const char* sampleCountToString(VkSampleCountFlagBits count);

} // end of namespace vkutil
