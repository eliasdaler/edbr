#include "VkPipelines.h"

#include <fstream>
#include <vector>

namespace vkutil
{
bool loadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule)
{
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    const auto fileSize = file.tellg();
    std::vector<std::uint32_t> buffer(fileSize / sizeof(std::uint32_t));

    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    file.close();

    auto info = VkShaderModuleCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = buffer.size() * sizeof(std::uint32_t),
        .pCode = buffer.data(),
    };

    if (vkCreateShaderModule(device, &info, nullptr, outShaderModule) != VK_SUCCESS) {
        return false;
    }

    return true;
}

} // end of namespace vkutil
