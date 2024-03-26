#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>

#include <edbr/Graphics/IdTypes.h>
#include <edbr/Graphics/Vulkan/GPUImage.h>

#include <edbr/Graphics/Vulkan/BindlessSetManager.h>

class GfxDevice;

class ImageCache {
    friend class ResourcesInspector;

public:
    ImageCache(GfxDevice& gfxDevice);

    ImageId loadImageFromFile(
        const std::filesystem::path& path,
        VkFormat format,
        VkImageUsageFlags usage,
        bool mipMap);

    ImageId addImage(GPUImage image);
    ImageId addImage(ImageId id, GPUImage image);
    const GPUImage& getImage(ImageId id) const;

    ImageId getFreeImageId() const;

    void destroyImages();

    BindlessSetManager bindlessSetManager;

private:
    std::vector<GPUImage> images;
    GfxDevice& gfxDevice;

    struct LoadedImageInfo {
        std::filesystem::path path;
        VkFormat format;
        VkImageUsageFlags usage;
        bool mipMap;
    };
    std::unordered_map<ImageId, LoadedImageInfo> loadedImagesInfo;
};
