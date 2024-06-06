#pragma once

#include <cassert>
#include <cstdint>
#include <optional>

#include <vulkan/vulkan.h>

#include <glm/vec4.hpp>

#define VK_CHECK(call)                 \
    do {                               \
        VkResult result_ = call;       \
        assert(result_ == VK_SUCCESS); \
    } while (0)

struct GPUImage;
struct GPUBuffer;
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
    VkExtent2D dstSize,
    VkFilter filter);

void copyImageToImage(
    VkCommandBuffer cmd,
    VkImage source,
    VkImage destination,
    VkExtent2D srcSize,
    int destX,
    int destY,
    int destW,
    int destH,
    VkFilter filter);

void generateMipmaps(
    VkCommandBuffer cmd,
    VkImage image,
    VkExtent2D imageSize,
    std::uint32_t mipLevels);

void cmdBeginLabel(VkCommandBuffer cmd, const char* label, glm::vec4 color = glm::vec4{1.f});
void cmdEndLabel(VkCommandBuffer cmd);

void addDebugLabel(VkDevice device, VkImage image, const char* label);
void addDebugLabel(VkDevice device, VkImageView imageView, const char* label);
void addDebugLabel(VkDevice device, VkShaderModule shaderModule, const char* label);
void addDebugLabel(VkDevice device, VkPipeline pipeline, const char* label);
void addDebugLabel(VkDevice device, VkPipelineLayout layout, const char* label);
void addDebugLabel(VkDevice device, VkBuffer buffer, const char* label);
void addDebugLabel(VkDevice device, VkSampler sampler, const char* label);

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

void clearColorImage(
    VkCommandBuffer cmd,
    VkExtent2D colorImageExtent,
    VkImageView colorImageView,
    const glm::vec4& clearColor);

int sampleCountToInt(VkSampleCountFlagBits count);
const char* sampleCountToString(VkSampleCountFlagBits count);

} // end of namespace vkutil
