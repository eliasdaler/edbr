#include "ImageCache.h"

#include "GfxDevice.h"

#include <iostream>

ImageCache::ImageCache(GfxDevice& gfxDevice) : gfxDevice(gfxDevice)
{}

ImageId ImageCache::loadImageFromFile(
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

    const auto id = getFreeImageId();
    auto image = gfxDevice.loadImageFromFile(path, format, usage, mipMap);
    addImage(id, std::move(image));

    loadedImagesInfo.emplace(
        id,
        LoadedImageInfo{
            .path = path,
            .format = format,
            .usage = usage,
            .mipMap = mipMap,
        });

    return id;
}

ImageId ImageCache::addImage(AllocatedImage image)
{
    return addImage(getFreeImageId(), std::move(image));
}

ImageId ImageCache::addImage(ImageId id, AllocatedImage image)
{
    image.setBindlessId(static_cast<std::uint32_t>(id));
    images.push_back(std::move(image));
    bindlessSetManager.addImage(gfxDevice.getDevice(), id, image.imageView);

    return id;
}

const AllocatedImage& ImageCache::getImage(ImageId id) const
{
    return images.at(id);
}

ImageId ImageCache::getFreeImageId() const
{
    return images.size();
}

void ImageCache::destroyImages()
{
    for (const auto& image : images) {
        gfxDevice.destroyImage(image);
    }
    images.clear();
    loadedImagesInfo.clear();
}
