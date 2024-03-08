#include "ImageCache.h"

#include "GfxDevice.h"

#include <iostream>

ImageId ImageCache::loadImageFromFile(
    GfxDevice& gfxDevice,
    const std::filesystem::path& path,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipMap)
{
    for (const auto& [id, info] : loadedImagesInfo) {
        // TODO: calculate some hash to not have to linear search every time?
        if (info.path == path && info.format == format && info.usage == usage &&
            info.mipMap == mipMap) {
            std::cout << "Already loaded: " << path << std::endl;
            return id;
        }
    }

    const auto imageId = getFreeImageId();
    auto image = gfxDevice.loadImageFromFile(path, format, usage, mipMap);
    addImage(imageId, std::move(image));

    loadedImagesInfo.emplace(
        imageId,
        LoadedImageInfo{
            .path = path,
            .format = format,
            .usage = usage,
            .mipMap = mipMap,
        });

    return imageId;
}

void ImageCache::addImage(ImageId id, AllocatedImage image)
{
    images.push_back(std::move(image));
}

const AllocatedImage& ImageCache::getImage(ImageId id) const
{
    return images.at(id);
}

ImageId ImageCache::getFreeImageId() const
{
    return images.size();
}

void ImageCache::cleanup(const GfxDevice& gfxDevice)
{
    for (const auto& image : images) {
        gfxDevice.destroyImage(image);
    }
    images.clear();
    loadedImagesInfo.clear();
}
