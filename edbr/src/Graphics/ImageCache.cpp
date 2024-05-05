#include <edbr/Graphics/ImageCache.h>

#include <edbr/Graphics/GfxDevice.h>

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

    auto image = gfxDevice.loadImageFromFileRaw(path, format, usage, mipMap);
    if (image.isInitialized() && image.getBindlessId() == errorImageId) {
        return errorImageId;
    }

    const auto id = getFreeImageId();
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

ImageId ImageCache::addImage(GPUImage image)
{
    return addImage(getFreeImageId(), std::move(image));
}

ImageId ImageCache::addImage(ImageId id, GPUImage image)
{
    image.setBindlessId(static_cast<std::uint32_t>(id));
    if (id != images.size()) {
        images[id] = std::move(image); // replacing existing image
    } else {
        images.push_back(std::move(image));
    }
    bindlessSetManager.addImage(gfxDevice.getDevice(), id, image.imageView);

    return id;
}

const GPUImage& ImageCache::getImage(ImageId id) const
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
