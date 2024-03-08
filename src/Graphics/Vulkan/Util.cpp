#include "Util.h"

#include <volk.h>

#include "Init.h"
#include "Types.h"
#include "VulkanImmediateExecutor.h"

#include <util/ImageLoader.h>

#include <cmath>
#include <cstdint>
#include <cstring>

namespace vkutil
{

AllocatedImage createImage(
    VkDevice device,
    VmaAllocator allocator,
    const CreateImageInfo& createInfo)
{
    std::uint32_t mipLevels = 1;
    if (createInfo.mipMap) {
        mipLevels = static_cast<std::uint32_t>(std::floor(
                        std::log2(std::max(createInfo.extent.width, createInfo.extent.height)))) +
                    1;
    }

    if (createInfo.isCubemap) {
        assert(createInfo.numLayers == 6);
        assert(!createInfo.mipMap);
        assert((createInfo.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) != 0);
    }

    const auto imgInfo = VkImageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = createInfo.flags,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = createInfo.format,
        .extent = createInfo.extent,
        .mipLevels = mipLevels,
        .arrayLayers = createInfo.numLayers,
        .samples = createInfo.samples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = createInfo.usage,
    };
    const auto allocInfo = VmaAllocationCreateInfo{
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    auto image = AllocatedImage{
        .format = createInfo.format,
        .usage = createInfo.usage,
        .extent = createInfo.extent,
        .mipLevels = mipLevels,
        .numLayers = createInfo.numLayers,
        .isCubemap = createInfo.isCubemap,
    };
    VK_CHECK(
        vmaCreateImage(allocator, &imgInfo, &allocInfo, &image.image, &image.allocation, nullptr));

    // create view
    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (createInfo.format == VK_FORMAT_D32_SFLOAT) { // TODO: support other depth formats
        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    auto viewType = createInfo.numLayers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    if (createInfo.isCubemap) {
        viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    }

    const auto viewCreateInfo = VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image.image,
        .viewType = viewType,
        .format = createInfo.format,
        .subresourceRange =
            VkImageSubresourceRange{
                .aspectMask = aspectFlag,
                .baseMipLevel = 0,
                .levelCount = mipLevels,
                .baseArrayLayer = 0,
                .layerCount = createInfo.numLayers,
            },
    };

    VK_CHECK(vkCreateImageView(device, &viewCreateInfo, nullptr, &image.imageView));

    return image;
}

void destroyImage(VkDevice device, VmaAllocator allocator, const AllocatedImage& image)
{
    vkDestroyImageView(device, image.imageView, nullptr);
    vmaDestroyImage(allocator, image.image, image.allocation);
}

void uploadImageData(
    VkDevice device,
    std::uint32_t graphicsQueueFamily,
    VkQueue graphicsQueue,
    VmaAllocator allocator,
    const AllocatedImage& image,
    void* pixelData,
    std::uint32_t layer)
{
    // FIXME: usage shader executor?
    // or allow image data to be uploaded in async way?
    VulkanImmediateExecutor executor;
    executor.init(device, graphicsQueueFamily, graphicsQueue);

    const auto dataSize = image.extent.depth * image.extent.width * image.extent.height * 4;

    const auto uploadBuffer =
        createBuffer(allocator, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO);
    memcpy(uploadBuffer.info.pMappedData, pixelData, dataSize);

    executor.immediateSubmit([&](VkCommandBuffer cmd) {
        assert(
            (image.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) != 0 &&
            "Image needs to have VK_IMAGE_USAGE_TRANSFER_DST_BIT to upload data to it");
        transitionImage(
            cmd, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        const auto copyRegion = VkBufferImageCopy{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = layer,
                    .layerCount = 1,
                },
            .imageExtent = image.extent,
        };

        vkCmdCopyBufferToImage(
            cmd,
            uploadBuffer.buffer,
            image.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion);

        if (image.mipLevels > 1) {
            assert(
                (image.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) != 0 &&
                (image.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0 &&
                "Image needs to have VK_IMAGE_USAGE_TRANSFER_{DST,SRC}_BIT to generate mip maps");
            generateMipmaps(
                cmd,
                image.image,
                VkExtent2D{image.extent.width, image.extent.height},
                image.mipLevels);
        } else {
            transitionImage(
                cmd,
                image.image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    });

    destroyBuffer(allocator, uploadBuffer);

    executor.cleanup(device);
}

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
                {},
                {(std::int32_t)dstSize.width, (std::int32_t)dstSize.height, 1},
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
        .filter = VK_FILTER_LINEAR,
    };

    vkCmdBlitImage2(cmd, &blitInfo);
}

void generateMipmaps(
    VkCommandBuffer cmd,
    VkImage image,
    VkExtent2D imageSize,
    std::uint32_t mipLevels)
{
    for (std::uint32_t mip = 0; mip < mipLevels; mip++) {
        const auto imageBarrier = VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .image = image,
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = mip,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };

        const auto depInfo = VkDependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &imageBarrier,
        };

        // TODO: check if inserting so many barriers matters for performance
        // maybe it's better to insert one barrier per image instead?
        vkCmdPipelineBarrier2(cmd, &depInfo);

        if (mip < mipLevels - 1) {
            const auto halfSize = VkExtent2D{
                .width = imageSize.width / 2,
                .height = imageSize.height / 2,
            };

            const auto blitRegion = VkImageBlit2{
                .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
                .srcSubresource =
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = mip,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                .srcOffsets =
                    {
                        {},
                        {(std::int32_t)imageSize.width, (std::int32_t)imageSize.height, 1},
                    },
                .dstSubresource =
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = mip + 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                .dstOffsets = {
                    {},
                    {(std::int32_t)halfSize.width, (std::int32_t)halfSize.height, 1},
                }};

            const auto blitInfo = VkBlitImageInfo2{
                .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
                .srcImage = image,
                .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .dstImage = image,
                .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .regionCount = 1,
                .pRegions = &blitRegion,
                .filter = VK_FILTER_LINEAR,
            };

            vkCmdBlitImage2(cmd, &blitInfo);

            imageSize = halfSize;
        }
    }

    // transition all mip levels into the final READ_ONLY layout
    transitionImage(
        cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

AllocatedImage loadImageFromFile(
    VkDevice device,
    std::uint32_t graphicsQueueFamily,
    VkQueue graphicsQueue,
    VmaAllocator allocator,
    const std::filesystem::path& path,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipMap)
{
    auto data = util::loadImage(path);
    assert(data.pixels);

    const auto image = createImage(
        device,
        allocator,
        {
            .format = format,
            .usage = usage | //
                     VK_IMAGE_USAGE_TRANSFER_DST_BIT | // for uploading pixel data to image
                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT, // for generating mips
            .extent =
                VkExtent3D{
                    .width = (std::uint32_t)data.width,
                    .height = (std::uint32_t)data.height,
                    .depth = 1,
                },
            .mipMap = mipMap,
        });
    uploadImageData(device, graphicsQueueFamily, graphicsQueue, allocator, image, data.pixels);

    vkutil::addDebugLabel(device, image.image, path.string().c_str());

    return image;
}

AllocatedBuffer createBuffer(
    VmaAllocator allocator,
    std::size_t allocSize,
    VkBufferUsageFlags usage,
    VmaMemoryUsage memoryUsage)
{
    const auto bufferInfo = VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = allocSize,
        .usage = usage,
    };

    const auto allocInfo = VmaAllocationCreateInfo{
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                 // TODO: allow to set VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT when needed
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = memoryUsage,
    };

    AllocatedBuffer buffer{};
    VK_CHECK(vmaCreateBuffer(
        allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.info));
    return buffer;
}

void destroyBuffer(VmaAllocator allocator, const AllocatedBuffer& buffer)
{
    vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}

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
