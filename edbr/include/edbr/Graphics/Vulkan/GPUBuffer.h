#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

struct GPUBuffer {
    VkBuffer buffer{VK_NULL_HANDLE};
    VmaAllocation allocation;
    VmaAllocationInfo info;

    // Only for buffers created with VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    // TODO: add check that this is not 0 if requesting address?
    VkDeviceAddress address{0};
};
