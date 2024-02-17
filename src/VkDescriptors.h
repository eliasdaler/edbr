#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include <vulkan/vulkan.h>

class DescriptorLayoutBuilder {
public:
    void addBinding(std::uint32_t binding, VkDescriptorType type);
    void clear();
    VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages);

private:
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};

class DescriptorAllocator {
public:
    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

public:
    void initPool(
        VkDevice device,
        std::uint32_t maxSets,
        std::span<const PoolSizeRatio> poolRatios);
    void clearDescriptors(VkDevice device);
    void destroyPool(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);

private:
    VkDescriptorPool pool;
};
