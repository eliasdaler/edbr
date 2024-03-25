#pragma once

#include <span>
#include <vector>

#include <vulkan/vulkan.h>

namespace vkutil
{
VkShaderModule loadShaderModule(const char* filePath, VkDevice device);
VkPipelineLayout createPipelineLayout(
    VkDevice device,
    std::span<const VkDescriptorSetLayout> layouts = {},
    std::span<const VkPushConstantRange> pushContantRanges = {});
} // end of namespace vkutil

class PipelineBuilder {
public:
    PipelineBuilder(VkPipelineLayout pipelineLayout);
    VkPipeline build(VkDevice device);

    PipelineBuilder& setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    PipelineBuilder& setShaders(
        VkShaderModule vertexShader,
        VkShaderModule geometryShader,
        VkShaderModule fragmentShader);
    PipelineBuilder& setInputTopology(VkPrimitiveTopology topology);
    PipelineBuilder& setPolygonMode(VkPolygonMode mode);
    PipelineBuilder& setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    PipelineBuilder& enableCulling(); // defaults for culling
    PipelineBuilder& disableCulling();
    PipelineBuilder& setMultisamplingNone();
    PipelineBuilder& setMultisampling(VkSampleCountFlagBits samples);
    PipelineBuilder& disableBlending();
    PipelineBuilder& enableBlending(
        VkBlendOp blendOp = VK_BLEND_OP_ADD,
        VkBlendFactor srcBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        VkBlendFactor dstBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    PipelineBuilder& setColorAttachmentFormat(VkFormat format);
    PipelineBuilder& setDepthFormat(VkFormat format);
    PipelineBuilder& enableDepthTest(bool depthWriteEnable, VkCompareOp op);
    PipelineBuilder& enableDepthClamp();
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

class ComputePipelineBuilder {
public:
    ComputePipelineBuilder(VkPipelineLayout pipelineLayout);
    ComputePipelineBuilder& setShader(VkShaderModule shaderModule);

    VkPipeline build(VkDevice device);

private:
    VkPipelineLayout pipelineLayout;
    VkShaderModule shaderModule;
};
