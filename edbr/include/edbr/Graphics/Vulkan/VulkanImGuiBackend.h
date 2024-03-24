#pragma once

#include <cstdint>

#include <glm/vec2.hpp>

#include <vulkan/vulkan.h>

#include <edbr/Graphics/IdTypes.h>
#include <edbr/Graphics/NBuffer.h>

class GfxDevice;

/*
 * This is a custom Dear ImGui rendering backend which does the following:
 * 1. Implements sRGB framebuffer support via various hacks outlined in vert.frag
 * 2. Doesn't use descriptor sets- the bindless texture IDs can be passed to ImGui::Image
 * 3. Properly displays _SRGB and other linear space textures
 *
 * Caveat: maximum idx/vtx count of 1 million - I'm too lazy to implement dynamic
 * idx/vtx buffer resizing.
 */
class VulkanImGuiBackend {
public:
    void init(GfxDevice& gfxDevice, VkFormat swapchainFormat);

    void draw(
        VkCommandBuffer cmd,
        GfxDevice& gfxDevice,
        VkImageView swapchainImageView,
        VkExtent2D swapchainExtent);

    void cleanup(GfxDevice& gfxDevice);

private:
    void copyBuffers(VkCommandBuffer cmd, GfxDevice& gfxDevice) const;

    NBuffer idxBuffer;
    NBuffer vtxBuffer;

    ImageId fontTextureId{NULL_IMAGE_ID};

    struct PushConstants {
        VkDeviceAddress vertexBuffer;
        std::uint32_t textureId;
        std::uint32_t textureIsSRGB;
        glm::vec2 translate;
        glm::vec2 scale;
    };

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
};
