#include "BindlessSetManager.h"

#include <array>

#include <volk.h>

#include <Graphics/Vulkan/Descriptors.h>
#include <Graphics/Vulkan/Util.h>

namespace
{
static const std::uint32_t maxBindlessResources = 16536;
static const std::uint32_t maxSamplers = 32;

static const std::uint32_t texturesBinding = 0;
static const std::uint32_t samplersBinding = 1;
}

void BindlessSetManager::init(VkDevice device)
{
    { // create pool
        const auto poolSizesBindless = std::array<VkDescriptorPoolSize, 2>{{
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxBindlessResources},
            {VK_DESCRIPTOR_TYPE_SAMPLER, maxSamplers},
        }};

        const auto poolInfo = VkDescriptorPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
            .maxSets = maxBindlessResources * poolSizesBindless.size(),
            .poolSizeCount = (std::uint32_t)poolSizesBindless.size(),
            .pPoolSizes = poolSizesBindless.data(),
        };

        VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descPool));
    }

    { // build desc set layout
        const auto bindings = std::array<VkDescriptorSetLayoutBinding, 2>{{
            {
                .binding = texturesBinding,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .descriptorCount = maxBindlessResources,
                .stageFlags = VK_SHADER_STAGE_ALL,
            },
            {
                .binding = samplersBinding,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                .descriptorCount = maxSamplers,
                .stageFlags = VK_SHADER_STAGE_ALL,
            },
        }};

        const VkDescriptorBindingFlags bindlessFlags =
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
        const auto bindingFlags = std::array{bindlessFlags, bindlessFlags};

        const auto flagInfo = VkDescriptorSetLayoutBindingFlagsCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .bindingCount = (std::uint32_t)bindingFlags.size(),
            .pBindingFlags = bindingFlags.data(),
        };

        const auto info = VkDescriptorSetLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = &flagInfo,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
            .bindingCount = (std::uint32_t)bindings.size(),
            .pBindings = bindings.data(),
        };

        VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &descSetLayout));
    }

    { // alloc desc set
        const auto allocInfo = VkDescriptorSetAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = descPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &descSetLayout,
        };

        std::uint32_t maxBinding = maxBindlessResources - 1;
        const auto countInfo = VkDescriptorSetVariableDescriptorCountAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
            .descriptorSetCount = 1,
            .pDescriptorCounts = &maxBinding,
        };

        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descSet));
    }
}

void BindlessSetManager::cleanup(VkDevice device)
{
    vkDestroyDescriptorSetLayout(device, descSetLayout, nullptr);
    vkDestroyDescriptorPool(device, descPool, nullptr);
}

void BindlessSetManager::addImage(
    const VkDevice device,
    std::uint32_t id,
    const VkImageView imageView)
{
    DescriptorWriter writer;
    writer.writeImage(
        texturesBinding,
        imageView,
        VK_NULL_HANDLE,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        id);
    writer.updateSet(device, descSet);
}

void BindlessSetManager::addSampler(const VkDevice device, std::uint32_t id, VkSampler sampler)
{
    DescriptorWriter writer;
    writer.writeSampler(samplersBinding, sampler, id);
    writer.updateSet(device, descSet);
}
