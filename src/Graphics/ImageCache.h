#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>

#include <Graphics/IdTypes.h>

#include <Graphics/Vulkan/Types.h>

class GfxDevice;

class ImageCache {
public:
    ImageId loadImageFromFile(
        GfxDevice& device,
        const std::filesystem::path& path,
        VkFormat format,
        VkImageUsageFlags usage,
        bool mipMap);

    void addImage(ImageId id, AllocatedImage image);
    const AllocatedImage& getImage(ImageId id) const;

    ImageId getFreeImageId() const;

    void cleanup(const GfxDevice& gfxDevice);

private:
    std::vector<AllocatedImage> images;

    struct LoadedImageInfo {
        std::filesystem::path path;
        VkFormat format;
        VkImageUsageFlags usage;
        bool mipMap;
    };
    std::unordered_map<ImageId, LoadedImageInfo> loadedImagesInfo;
};
