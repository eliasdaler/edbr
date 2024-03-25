#pragma once

#include <array>
#include <unordered_map>

#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <im3d.h>

#include <edbr/Graphics/Camera.h>
#include <edbr/Graphics/NBuffer.h>

class GfxDevice;

class Im3dState {
public:
    void init(GfxDevice& device, VkFormat drawImageFormat);

    void cleanup(GfxDevice& gfxDevice);
    void newFrame(
        float dt,
        const glm::vec2& vpSize,
        const Camera& camera,
        const glm::ivec2& mousePos,
        bool leftMousePressed);
    void draw(
        VkCommandBuffer cmd,
        GfxDevice& gfxDevice,
        VkImageView swapchainImageView,
        VkExtent2D swapchainExtent);

    void endFrame();

    void drawText(
        const glm::mat4& viewProj,
        const glm::ivec2& gameWindowPos,
        const glm::ivec2& gameWindowSize);

    struct RenderState {
        bool clearDepth{false};
        glm::mat4 viewProj;
        std::array<int, 4> viewport{};

        bool hasCustomViewport() const { return viewport != std::array<int, 4>{}; }
    };

    static Im3d::Id DefaultLayer;
    static Im3d::Id WorldWithDepthLayer;
    static Im3d::Id WorldNoDepthLayer;

    void addRenderState(Im3d::Id layerId, RenderState state)
    {
        renderStates.try_emplace(layerId, state);
    }

    void updateCameraViewProj(Im3d::Id layerId, const glm::mat4& viewProj)
    {
        renderStates.at(layerId).viewProj = viewProj;
    }

private:
    void drawTextDrawListsImGui(
        const Im3d::TextDrawList _textDrawLists[],
        std::uint32_t _count,
        const glm::mat4& viewProj,
        const glm::ivec2& gameWindowPos,
        const glm::ivec2& gameWindowSize);

    void copyVertices(VkCommandBuffer cmd, GfxDevice& gfxDevice);

    std::unordered_map<Im3d::Id, RenderState> renderStates;

    struct PushConstants {
        VkDeviceAddress vertexBuffer;
        glm::mat4 viewProj;
        glm::vec2 viewport;
    };

    VkPipelineLayout pointsPipelineLayout;
    VkPipeline pointsPipeline;

    VkPipelineLayout linesPipelineLayout;
    VkPipeline linesPipeline;

    VkPipelineLayout trianglesPipelineLayout;
    VkPipeline trianglesPipeline;

    NBuffer vtxBuffer;
};
