#include <edbr/DevTools/ScreenshotTaker.h>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/printf.h>

#include <cassert>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/Vulkan/Util.h>

void ScreenshotTaker::setScreenshotDir(std::filesystem::path dir)
{
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
        if (!std::filesystem::exists(dir)) {
            throw std::runtime_error(fmt::format("failed to create dir '{}'", dir.string()));
        }
    }

    screenshotDir = std::move(dir);
}

void ScreenshotTaker::takeScreenshot(GfxDevice& gfxDevice, ImageId drawImageId)
{
    assert(drawImageId != NULL_IMAGE_ID);

    // round to seconds to not get milliseconds in filename
    auto now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());

    const auto filename = fmt::format("{:%Y-%m-%d_%H-%M-%S}.png", now);
    auto screenshotDest = screenshotDir / filename;

    // save with milliseconds if taking screenshots fast
    if (std::filesystem::exists(screenshotDest)) {
        const auto now = std::chrono::system_clock::now();
        const auto filename = fmt::format("{:%Y-%m-%d_%H-%M-%S}.png", now);
        screenshotDest = screenshotDir / filename;
        if (std::filesystem::exists(screenshotDest)) {
            // TODO: can probably save with (2) or (3), (4) etc. added
            // but whatever
            fmt::println("[dev] failed to save to {}: file exists", screenshotDest.string());
            return;
        }
    }

    fmt::println("[dev] saving screenshot to '{}'", screenshotDest.string());

    const auto& srcImg = gfxDevice.getImage(drawImageId);

    // Create dstImg - mapped and tiling should be linear
    const auto createImageInfo = vkutil::CreateImageInfo{
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .extent =
            {
                .width = (std::uint32_t)srcImg.extent.width,
                .height = (std::uint32_t)srcImg.extent.height,
                .depth = 1,
            },
        .tiling = VK_IMAGE_TILING_LINEAR,
    };
    const auto allocInfo = VmaAllocationCreateInfo{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                 VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
    };
    const auto dstImg = gfxDevice.createImageRaw(createImageInfo, allocInfo);

    // copy image to dstImg
    gfxDevice.immediateSubmit([srcImg, dstImg](VkCommandBuffer cmd) {
        // sync with draw - but same layout
        vkutil::transitionImage(
            cmd,
            srcImg.image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        vkutil::transitionImage(
            cmd, dstImg.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vkutil::copyImageToImage(
            cmd,
            srcImg.image,
            dstImg.image,
            srcImg.getExtent2D(),
            dstImg.getExtent2D(),
            VK_FILTER_NEAREST);
    });

    // Get info about where to read pixels from and how
    VkImageSubresource subResource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
    VkSubresourceLayout subResourceLayout;
    vkGetImageSubresourceLayout(
        gfxDevice.getDevice(), dstImg.image, &subResource, &subResourceLayout);

    VmaAllocationInfo info;
    vmaGetAllocationInfo(gfxDevice.getAllocator(), dstImg.allocation, &info);

    const char* data = (const char*)info.pMappedData;
    data += subResourceLayout.offset;

    stbi_write_force_png_filter = 0; // this speeds up image write considerably
    int numChannels = 4;
    const auto res = stbi_write_png(
        screenshotDest.string().c_str(),
        dstImg.extent.width,
        dstImg.extent.height,
        numChannels,
        data,
        subResourceLayout.rowPitch);

    if (res == 0) {
        fmt::println("[dev] failed to save screenshot");
    }

    gfxDevice.destroyImage(dstImg);
}
