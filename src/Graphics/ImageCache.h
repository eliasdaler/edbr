#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>

#include <Graphics/IdTypes.h>

#include <Graphics/Vulkan/BindlessSetManager.h>
#include <Graphics/Vulkan/Types.h>

class GfxDevice;

class ImageCache {
public:
    ImageCache(GfxDevice& gfxDevice);

    ImageId loadImageFromFile(
        const std::filesystem::path& path,
        VkFormat format,
        VkImageUsageFlags usage,
        bool mipMap);

    ImageId addImage(AllocatedImage image);
    ImageId addImage(ImageId id, AllocatedImage image);
    const AllocatedImage& getImage(ImageId id) const;

    ImageId getFreeImageId() const;

    void destroyImages();

    BindlessSetManager bindlessSetManager;

private:
    std::vector<AllocatedImage> images;
    GfxDevice& gfxDevice;

    struct LoadedImageInfo {
        std::filesystem::path path;
        VkFormat format;
        VkImageUsageFlags usage;
        bool mipMap;
    };
    std::unordered_map<ImageId, LoadedImageInfo> loadedImagesInfo;
};
