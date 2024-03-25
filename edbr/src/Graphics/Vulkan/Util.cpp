#include <edbr/Graphics/Vulkan/Util.h>

#include <volk.h>

#include <edbr/Graphics/Vulkan/GPUBuffer.h>
#include <edbr/Graphics/Vulkan/GPUImage.h>
#include <edbr/Graphics/Vulkan/Init.h>

#include <cstdint>

namespace vkutil
{

void transitionImage(
    VkCommandBuffer cmd,
    VkImage image,
    VkImageLayout currentLayout,
    VkImageLayout newLayout)
{
    VkImageAspectFlags aspectMask =
        (currentLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ||
         newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ||
         newLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL) ?
            VK_IMAGE_ASPECT_DEPTH_BIT :
            VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageMemoryBarrier2 imageBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
        .oldLayout = currentLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange = vkinit::imageSubresourceRange(aspectMask),
    };

    VkDependencyInfo depInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier,
    };

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void copyImageToImage(
    VkCommandBuffer cmd,
    VkImage source,
    VkImage destination,
    VkExtent2D srcSize,
    VkExtent2D dstSize)
{
    copyImageToImage(cmd, source, destination, srcSize, 0, 0, dstSize.width, dstSize.height);
}

void copyImageToImage(
    VkCommandBuffer cmd,
    VkImage source,
    VkImage destination,
    VkExtent2D srcSize,
    int destX,
    int destY,
    int destW,
    int destH)
{
    const auto blitRegion = VkImageBlit2{
        .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
        .srcSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        .srcOffsets =
            {
                {},
                {(std::int32_t)srcSize.width, (std::int32_t)srcSize.height, 1},
            },
        .dstSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        .dstOffsets =
            {
                {(std::int32_t)destX, (std::int32_t)destY},
                {(std::int32_t)(destX + destW), (std::int32_t)(destY + destH), 1},
            },
    };

    const auto blitInfo = VkBlitImageInfo2{
        .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
        .srcImage = source,
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstImage = destination,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount = 1,
        .pRegions = &blitRegion,
        .filter = VK_FILTER_LINEAR, // TODO: allow to specify
    };

    vkCmdBlitImage2(cmd, &blitInfo);
}

void generateMipmaps(
    VkCommandBuffer cmd,
    VkImage image,
    VkExtent2D imageSize,
    std::uint32_t mipLevels)
{}

void cmdBeginLabel(VkCommandBuffer cmd, const char* label, glm::vec4 color)
{
    const VkDebugUtilsLabelEXT cmdLabel = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = label,
        .color = {color.x, color.y, color.z, color.w},
    };

    vkCmdBeginDebugUtilsLabelEXT(cmd, &cmdLabel);
}

void cmdEndLabel(VkCommandBuffer cmd)
{
    vkCmdEndDebugUtilsLabelEXT(cmd);
}

void addDebugLabel(VkDevice device, VkImage image, const char* label)
{
    const auto nameInfo = VkDebugUtilsObjectNameInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_IMAGE,
        .objectHandle = (std::uint64_t)image,
        .pObjectName = label,
    };
    vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
}

void addDebugLabel(VkDevice device, VkImageView imageView, const char* label)
{
    const auto nameInfo = VkDebugUtilsObjectNameInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_IMAGE_VIEW,
        .objectHandle = (std::uint64_t)imageView,
        .pObjectName = label,
    };
    vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
}

void addDebugLabel(VkDevice device, VkShaderModule shaderModule, const char* label)
{
    const auto nameInfo = VkDebugUtilsObjectNameInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_SHADER_MODULE,
        .objectHandle = (std::uint64_t)shaderModule,
        .pObjectName = label,
    };
    vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
}

void addDebugLabel(VkDevice device, VkPipeline pipeline, const char* label)
{
    const auto nameInfo = VkDebugUtilsObjectNameInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_PIPELINE,
        .objectHandle = (std::uint64_t)pipeline,
        .pObjectName = label,
    };
    vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
}

void addDebugLabel(VkDevice device, VkPipelineLayout layout, const char* label)
{
    const auto nameInfo = VkDebugUtilsObjectNameInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT,
        .objectHandle = (std::uint64_t)layout,
        .pObjectName = label,
    };
    vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
}

void addDebugLabel(VkDevice device, VkBuffer buffer, const char* label)
{
    const auto nameInfo = VkDebugUtilsObjectNameInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_BUFFER,
        .objectHandle = (std::uint64_t)buffer,
        .pObjectName = label,
    };
    vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
}

void addDebugLabel(VkDevice device, VkSampler sampler, const char* label)
{
    const auto nameInfo = VkDebugUtilsObjectNameInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_SAMPLER,
        .objectHandle = (std::uint64_t)sampler,
        .pObjectName = label,
    };
    vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
}

RenderInfo createRenderingInfo(const RenderingInfoParams& params)
{
    assert(
        (params.colorImageView || params.depthImageView != nullptr) &&
        "Either draw or depth image should be present");
    assert(
        params.renderExtent.width != 0.f && params.renderExtent.height != 0.f &&
        "renderExtent not specified");

    RenderInfo ri;
    if (params.colorImageView) {
        ri.colorAttachment = VkRenderingAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = params.colorImageView,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = params.colorImageClearValue ? VK_ATTACHMENT_LOAD_OP_CLEAR :
                                                    VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        };
        if (params.colorImageClearValue) {
            const auto col = params.colorImageClearValue.value();
            ri.colorAttachment.clearValue.color = {col[0], col[1], col[2], col[3]};
        }
    }

    if (params.depthImageView) {
        ri.depthAttachment = VkRenderingAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = params.depthImageView,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp = params.depthImageClearValue ? VK_ATTACHMENT_LOAD_OP_CLEAR :
                                                    VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        };
        if (params.depthImageClearValue) {
            ri.depthAttachment.clearValue.depthStencil.depth = params.depthImageClearValue.value();
        }
    }

    if (params.resolveImageView) {
        ri.colorAttachment.resolveImageView = params.resolveImageView;
        ri.colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ri.colorAttachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
    }

    ri.renderingInfo = VkRenderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea =
            VkRect2D{
                .offset = {},
                .extent = params.renderExtent,
            },
        .layerCount = 1,
        .colorAttachmentCount = params.colorImageView ? 1u : 0u,
        .pColorAttachments = params.colorImageView ? &ri.colorAttachment : nullptr,
        .pDepthAttachment = params.depthImageView ? &ri.depthAttachment : nullptr,
    };

    return ri;
}

int sampleCountToInt(VkSampleCountFlagBits count)
{
    switch (count) {
    case VK_SAMPLE_COUNT_1_BIT:
        return 1;
    case VK_SAMPLE_COUNT_2_BIT:
        return 2;
    case VK_SAMPLE_COUNT_4_BIT:
        return 4;
    case VK_SAMPLE_COUNT_8_BIT:
        return 8;
    case VK_SAMPLE_COUNT_16_BIT:
        return 16;
    case VK_SAMPLE_COUNT_32_BIT:
        return 32;
    case VK_SAMPLE_COUNT_64_BIT:
        return 64;
    default:
        return 0;
    }
}

const char* sampleCountToString(VkSampleCountFlagBits count)
{
    switch (count) {
    case VK_SAMPLE_COUNT_1_BIT:
        return "Off";
    case VK_SAMPLE_COUNT_2_BIT:
        return "2x";
    case VK_SAMPLE_COUNT_4_BIT:
        return "4x";
    case VK_SAMPLE_COUNT_8_BIT:
        return "8x";
    case VK_SAMPLE_COUNT_16_BIT:
        return "16x";
    case VK_SAMPLE_COUNT_32_BIT:
        return "32x";
    case VK_SAMPLE_COUNT_64_BIT:
        return "64x";
    default:
        return "Unknown";
    }
}

} // end of namespace vkutil
