#pragma once

#include <vulkan/vulkan.h>

namespace vkutil
{
bool loadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
} // end of namespace vkutil
