#pragma once

#include <cassert>
#include <cstdint>
#include <limits>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>

struct GPUImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkFormat format;
    VkImageUsageFlags usage;
    VkExtent3D extent;
    std::uint32_t mipLevels{1};
    std::uint32_t numLayers{1};
    bool isCubemap{false};

    static const auto NULL_BINDLESS_ID = std::numeric_limits<std::uint32_t>::max();

    glm::ivec2 getSize2D() const { return glm::ivec2{extent.width, extent.height}; }
    VkExtent2D getExtent2D() const { return VkExtent2D{extent.width, extent.height}; }

    std::uint32_t getBindlessId() const
    {
        assert(id != NULL_BINDLESS_ID && "Image wasn't added to bindless set");
        return id;
    }

    // should be called by ImageCache only
    void setBindlessId(const std::uint32_t id)
    {
        assert(id != NULL_BINDLESS_ID);
        this->id = id;
    }

private:
    std::uint32_t id{NULL_BINDLESS_ID}; // bindless id - always equals to ImageId
};
