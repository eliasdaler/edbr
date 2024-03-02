#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <vulkan/vulkan.h>

class Renderer;
struct AllocatedImage;

class BackgroundGradientPipeline {
public:
    void init(Renderer& renderer, const AllocatedImage& drawImage);
    void cleanup(VkDevice device);

    void draw(VkCommandBuffer cmd, const glm::vec2& imageSize);

private:
    VkDescriptorSetLayout drawImageDescriptorLayout;
    VkDescriptorSet drawImageDescriptors;
    VkPipelineLayout gradientPipelineLayout;
    VkPipeline gradientPipeline;

    struct ComputePushConstants {
        glm::vec4 data1;
        glm::vec4 data2;
        glm::vec4 data3;
        glm::vec4 data4;
    };
    ComputePushConstants gradientConstants;
};
