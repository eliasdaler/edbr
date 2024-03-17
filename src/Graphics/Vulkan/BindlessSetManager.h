#pragma once

#include <cstdint>

#include <vulkan/vulkan.h>

struct AllocatedImage;

class BindlessSetManager {
public:
    void init(VkDevice device);
    void cleanup(VkDevice device);

    VkDescriptorSetLayout getDescSetLayout() const { return descSetLayout; }
    VkDescriptorSet getDescSet() const { return descSet; }

    void addImage(VkDevice device, std::uint32_t id, const VkImageView imageView);
    void addSampler(VkDevice device, std::uint32_t id, VkSampler sampler);

private:
    VkDescriptorPool descPool;
    VkDescriptorSetLayout descSetLayout;
    VkDescriptorSet descSet;
};
