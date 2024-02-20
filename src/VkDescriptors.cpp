#include "VkDescriptors.h"

#include <VkUtil.h>

#include <cstdint>

void DescriptorLayoutBuilder::addBinding(std::uint32_t binding, VkDescriptorType type)
{
    bindings.push_back(VkDescriptorSetLayoutBinding{
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = 1,
    });
}

void DescriptorLayoutBuilder::clear()
{
    bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::build(
    VkDevice device,
    VkShaderStageFlags shaderStages)
{
    for (auto& b : bindings) {
        b.stageFlags |= shaderStages;
    }

    auto info = VkDescriptorSetLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = (std::uint32_t)bindings.size(),
        .pBindings = bindings.data(),
    };

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));
    return set;
}

void DescriptorAllocator::initPool(
    VkDevice device,
    std::uint32_t maxSets,
    std::span<const PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(poolRatios.size());
    for (const auto& ratio : poolRatios) {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = ratio.type,
            .descriptorCount = (std::uint32_t)(ratio.ratio * maxSets),
        });
    }

    auto info = VkDescriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = maxSets,
        .poolSizeCount = (std::uint32_t)poolSizes.size(),
        .pPoolSizes = poolSizes.data(),
    };

    VK_CHECK(vkCreateDescriptorPool(device, &info, nullptr, &pool));
}

void DescriptorAllocator::clearDescriptors(VkDevice device)
{
    VK_CHECK(vkResetDescriptorPool(device, pool, 0));
}

void DescriptorAllocator::destroyPool(VkDevice device)
{
    vkDestroyDescriptorPool(device, pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout)
{
    const auto info = VkDescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    VkDescriptorSet set;
    VK_CHECK(vkAllocateDescriptorSets(device, &info, &set));
    return set;
}

void DescriptorAllocatorGrowable::init(
    VkDevice device,
    std::uint32_t maxSets,
    std::span<const PoolSizeRatio> poolRatios)
{
    ratios.clear();

    for (const auto& r : poolRatios) {
        ratios.push_back(r);
    }

    auto newPool = createPool(device, maxSets, poolRatios);
    setsPerPool = maxSets * 1.5;
    readyPools.push_back(newPool);
}

void DescriptorAllocatorGrowable::clearPools(VkDevice device)
{
    for (const auto& p : readyPools) {
        vkResetDescriptorPool(device, p, 0);
    }
    for (const auto& p : fullPools) {
        vkResetDescriptorPool(device, p, 0);
        readyPools.push_back(p);
    }
    fullPools.clear();
}

void DescriptorAllocatorGrowable::destroyPools(VkDevice device)
{
    for (const auto& p : readyPools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    readyPools.clear();
    for (const auto& p : fullPools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    fullPools.clear();
}

VkDescriptorSet DescriptorAllocatorGrowable::allocate(VkDevice device, VkDescriptorSetLayout layout)
{
    auto poolToUse = getPool(device);

    auto allocInfo = VkDescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = poolToUse,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    VkDescriptorSet ds;
    const VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);
    // Allocate new pool is ran out of memory
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
        fullPools.push_back(poolToUse);

        poolToUse = getPool(device);
        allocInfo.descriptorPool = poolToUse;

        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));
    }

    readyPools.push_back(poolToUse);
    return ds;
}

VkDescriptorPool DescriptorAllocatorGrowable::getPool(VkDevice device)
{
    VkDescriptorPool pool;
    if (readyPools.size() != 0) {
        pool = readyPools.back();
        readyPools.pop_back();
    } else {
        // need to create a new pool
        pool = createPool(device, setsPerPool, ratios);

        setsPerPool = setsPerPool * 1.5;
        if (setsPerPool > 4096) {
            setsPerPool = 4096;
        }
    }
    return pool;
}

VkDescriptorPool DescriptorAllocatorGrowable::createPool(
    VkDevice device,
    std::uint32_t setCount,
    std::span<const PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(poolRatios.size());
    for (const auto& ratio : poolRatios) {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = ratio.type,
            .descriptorCount = (std::uint32_t)(ratio.ratio * setCount),
        });
    }

    const auto poolInfo = VkDescriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = 0,
        .maxSets = setCount,
        .poolSizeCount = (std::uint32_t)poolSizes.size(),
        .pPoolSizes = poolSizes.data(),
    };

    VkDescriptorPool pool;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool);
    return pool;
}

void DescriptorWriter::writeBuffer(
    std::uint32_t binding,
    VkBuffer buffer,
    size_t size,
    size_t offset,
    VkDescriptorType type)
{
    auto& info = bufferInfos.emplace_back(
        VkDescriptorBufferInfo{.buffer = buffer, .offset = offset, .range = size});

    writes.push_back(VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = VK_NULL_HANDLE, // will set in updateDe
        .dstBinding = binding,
        .descriptorCount = 1,
        .descriptorType = type,
        .pBufferInfo = &info,
    });
}

void DescriptorWriter::writeImage(
    std::uint32_t binding,
    VkImageView image,
    VkSampler sampler,
    VkImageLayout layout,
    VkDescriptorType type)
{
    auto& info = imageInfos.emplace_back(
        VkDescriptorImageInfo{.sampler = sampler, .imageView = image, .imageLayout = layout});

    writes.push_back(VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = VK_NULL_HANDLE, // will set in updateSet
        .dstBinding = binding,
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = &info,
    });
}

void DescriptorWriter::clear()
{
    imageInfos.clear();
    writes.clear();
    bufferInfos.clear();
}

void DescriptorWriter::updateSet(VkDevice device, VkDescriptorSet set)
{
    for (auto& write : writes) {
        write.dstSet = set;
    }

    vkUpdateDescriptorSets(device, (std::uint32_t)writes.size(), writes.data(), 0, nullptr);
}
