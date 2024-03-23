#pragma once

#include <cstdint>
#include <deque>
#include <span>
#include <vector>

#include <vulkan/vulkan.h>

struct DescriptorLayoutBinding {
    std::uint32_t binding;
    VkDescriptorType type;
};

namespace vkutil
{
VkDescriptorSetLayout buildDescriptorSetLayout(
    VkDevice device,
    VkShaderStageFlags shaderStages,
    std::span<const DescriptorLayoutBinding> bindings);
}

class DescriptorAllocatorGrowable {
public:
    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

    void init(
        VkDevice device,
        std::uint32_t initialSets,
        std::span<const PoolSizeRatio> poolRatios);
    void clearPools(VkDevice device);
    void destroyPools(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);

private:
    VkDescriptorPool getPool(VkDevice device);
    VkDescriptorPool createPool(
        VkDevice device,
        std::uint32_t setCount,
        std::span<const PoolSizeRatio> poolRatios);

    std::vector<PoolSizeRatio> ratios;
    std::vector<VkDescriptorPool> fullPools;
    std::vector<VkDescriptorPool> readyPools;
    std::uint32_t setsPerPool;
};

class DescriptorWriter {
public:
    void writeBuffer(
        std::uint32_t binding,
        VkBuffer buffer,
        size_t size,
        size_t offset,
        VkDescriptorType type);
    void writeImage(
        std::uint32_t binding,
        VkImageView image,
        VkSampler sampler,
        VkImageLayout layout,
        VkDescriptorType type,
        std::uint32_t dstArrayElement = 0);
    void writeSampler(std::uint32_t binding, VkSampler sampler, std::uint32_t dstArrayElement = 0);

    void clear();
    void updateSet(VkDevice device, VkDescriptorSet set);

private:
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::deque<VkDescriptorImageInfo> imageInfos;
    std::vector<VkWriteDescriptorSet> writes;
};
