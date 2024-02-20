#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace vkutil
{
bool loadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
} // end of namespace vkutil

class PipelineBuilder {
public:
    PipelineBuilder(VkPipelineLayout pipelineLayout);
    VkPipeline build(VkDevice device);

    PipelineBuilder& setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    PipelineBuilder& setInputTopology(VkPrimitiveTopology topology);
    PipelineBuilder& setPolygonMode(VkPolygonMode mode);
    PipelineBuilder& setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    PipelineBuilder& setMultisamplingNone();
    PipelineBuilder& disableBlending();
    PipelineBuilder& setColorAttachmentFormat(VkFormat format);
    PipelineBuilder& setDepthFormat(VkFormat format);
    PipelineBuilder& enableDepthTest(bool depthWriteEnable, VkCompareOp op);
    PipelineBuilder& disableDepthTest();

private:
    void clear();

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    VkPipelineRenderingCreateInfo renderInfo;
    VkFormat colorAttachmentformat;
    VkPipelineLayout pipelineLayout;
};
