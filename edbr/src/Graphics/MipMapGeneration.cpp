#include <edbr/Graphics/MipMapGeneration.h>

#include <algorithm>

#include <volk.h>

#include <edbr/Graphics/Vulkan/Util.h>

namespace graphics
{
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
                .width = std::max(imageSize.width / 2, 1u),
                .height = std::max(imageSize.height / 2, 1u),
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
    vkutil::transitionImage(
        cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

}
