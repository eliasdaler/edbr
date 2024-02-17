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
    auto info = VkDescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    VkDescriptorSet set;
    VK_CHECK(vkAllocateDescriptorSets(device, &info, &set));
    return set;
}
