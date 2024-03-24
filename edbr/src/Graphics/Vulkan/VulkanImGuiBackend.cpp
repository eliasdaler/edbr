#include <edbr/Graphics/Vulkan/VulkanImGuiBackend.h>

#include <edbr/Graphics/Common.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/Vulkan/Pipelines.h>
#include <edbr/Graphics/Vulkan/Util.h>

#include <imgui.h>

namespace
{
static const int MAX_IDX_COUNT = 1000000;
static const int MAX_VTX_COUNT = 1000000;

// this is the format we expect
struct ImGuiVertexFormat {
    glm::vec2 position;
    glm::vec2 uv;
    std::uint32_t color;
};
static_assert(
    sizeof(ImDrawVert) == sizeof(ImGuiVertexFormat),
    "ImDrawVert and ImGuiVertexFormat size mismatch");
static_assert(
    sizeof(ImDrawIdx) == sizeof(std::uint16_t) || sizeof(ImDrawIdx) == sizeof(std::uint32_t),
    "Only uint16_t or uint32_t indices are supported");

std::uint32_t toBindlessTextureId(ImTextureID id)
{
    return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(id));
}
}

void VulkanImGuiBackend::init(GfxDevice& gfxDevice, VkFormat swapchainFormat)
{
    idxBuffer.init(
        gfxDevice,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        sizeof(ImDrawIdx) * MAX_IDX_COUNT,
        graphics::FRAME_OVERLAP,
        "ImGui index buffer");
    vtxBuffer.init(
        gfxDevice,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        sizeof(ImDrawVert) * MAX_VTX_COUNT,
        graphics::FRAME_OVERLAP,
        "ImGui vertex buffer");

    auto& io = ImGui::GetIO();
    io.BackendRendererName = "EDBR ImGui Backend";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    // upload font
    {
        auto* pixels = static_cast<std::uint8_t*>(nullptr);
        int width = 0;
        int height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        fontTextureId = gfxDevice.createImage(
            {
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .extent = VkExtent3D{(std::uint32_t)width, (std::uint32_t)height, 1},
            },
            "ImGui font texture",
            pixels);
        io.Fonts->SetTexID(reinterpret_cast<ImTextureID>((std::uint64_t)fontTextureId));
    }

    const auto& device = gfxDevice.getDevice();

    const auto vertexShader = vkutil::loadShaderModule("shaders/imgui.vert.spv", device);
    const auto fragShader = vkutil::loadShaderModule("shaders/imgui.frag.spv", device);

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    const auto layouts = std::array{gfxDevice.getBindlessDescSetLayout()};
    const auto pushConstantRanges = std::array{bufferRange};
    pipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);
    vkutil::addDebugLabel(device, pipelineLayout, "ImGui pipeline layout");

    pipeline = PipelineBuilder{pipelineLayout}
                   .setShaders(vertexShader, fragShader)
                   .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                   .setPolygonMode(VK_POLYGON_MODE_FILL)
                   .disableCulling()
                   .setMultisamplingNone()
                   // see imgui.frag for explanation of this blending state
                   .enableBlending(
                       VK_BLEND_OP_ADD,
                       VK_BLEND_FACTOR_ONE,
                       VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                       VK_BLEND_FACTOR_ONE,
                       VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
                   .setColorAttachmentFormat(swapchainFormat)
                   .disableDepthTest()
                   .build(device);
    vkutil::addDebugLabel(device, pipeline, "ImGui pipeline");

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);
}

void VulkanImGuiBackend::draw(
    VkCommandBuffer cmd,
    GfxDevice& gfxDevice,
    VkImageView swapchainImageView,
    VkExtent2D swapchainExtent)
{
    const auto* drawData = ImGui::GetDrawData();
    assert(drawData);
    if (drawData->TotalVtxCount == 0) {
        return;
    }

    if (drawData->TotalIdxCount > MAX_IDX_COUNT || drawData->TotalVtxCount > MAX_VTX_COUNT) {
        printf(
            "VulkanImGuiBackend: too many vertices/indices to render (max indices = %d, max "
            "vertices = %d), buffer resize is not yet implemented.\n",
            MAX_IDX_COUNT,
            MAX_VTX_COUNT);
        assert(false && "TODO: implement ImGui buffer resize");
        return;
    }

    copyBuffers(cmd, gfxDevice);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    gfxDevice.bindBindlessDescSet(cmd, pipelineLayout);

    const auto renderInfo = vkutil::createRenderingInfo({
        .renderExtent = swapchainExtent,
        .colorImageView = swapchainImageView,
    });
    vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

    const auto targetWidth = (float)swapchainExtent.width;
    const auto targetHeight = (float)swapchainExtent.height;
    const auto viewport = VkViewport{
        .x = 0,
        .y = 0,
        .width = targetWidth,
        .height = targetHeight,
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    const auto clipOffset = drawData->DisplayPos;
    const auto clipScale = drawData->FramebufferScale;

    std::size_t globalIdxOffset = 0;
    std::size_t globalVtxOffset = 0;

    vkCmdBindIndexBuffer(
        cmd,
        idxBuffer.getBuffer().buffer,
        0,
        sizeof(ImDrawIdx) == sizeof(std::uint16_t) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

    for (int cmdListID = 0; cmdListID < drawData->CmdListsCount; ++cmdListID) {
        const auto& cmdList = *drawData->CmdLists[cmdListID];
        for (int cmdID = 0; cmdID < cmdList.CmdBuffer.Size; ++cmdID) {
            const auto& imCmd = cmdList.CmdBuffer[cmdID];
            if (imCmd.UserCallback) {
                if (imCmd.UserCallback != ImDrawCallback_ResetRenderState) {
                    imCmd.UserCallback(&cmdList, &imCmd);
                    continue;
                }
                assert(false && "ImDrawCallback_ResetRenderState is not supported");
            }

            if (imCmd.ElemCount == 0) {
                continue;
            }

            auto clipMin = ImVec2(
                (imCmd.ClipRect.x - clipOffset.x) * clipScale.x,
                (imCmd.ClipRect.y - clipOffset.y) * clipScale.y);
            auto clipMax = ImVec2(
                (imCmd.ClipRect.z - clipOffset.x) * clipScale.x,
                (imCmd.ClipRect.w - clipOffset.y) * clipScale.y);
            clipMin.x = std::clamp(clipMin.x, 0.0f, targetWidth);
            clipMax.x = std::clamp(clipMax.x, 0.0f, targetWidth);
            clipMin.y = std::clamp(clipMin.y, 0.0f, targetHeight);
            clipMax.y = std::clamp(clipMax.y, 0.0f, targetHeight);

            if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y) {
                continue;
            }

            auto textureId = gfxDevice.getWhiteTextureID();
            if (imCmd.TextureId != ImTextureID()) {
                textureId = toBindlessTextureId(imCmd.TextureId);
                assert(textureId != NULL_IMAGE_ID);
            }
            bool textureIsSRGB = true;
            const auto& texture = gfxDevice.getImage(textureId);
            if (texture.format == VK_FORMAT_R8G8B8A8_SRGB ||
                texture.format == VK_FORMAT_R16G16B16A16_SFLOAT) {
                // TODO: support more formats?
                textureIsSRGB = false;
            }

            const auto scale =
                glm::vec2(2.0f / drawData->DisplaySize.x, 2.0f / drawData->DisplaySize.y);
            const auto translate = glm::vec2(
                -1.0f - drawData->DisplayPos.x * scale.x, -1.0f - drawData->DisplayPos.y * scale.y);

            // set scissor
            const auto scissorX = static_cast<std::int32_t>(clipMin.x);
            const auto scissorY = static_cast<std::int32_t>(clipMin.y);
            const auto scissorWidth = static_cast<std::uint32_t>(clipMax.x - clipMin.x);
            const auto scissorHeight = static_cast<std::uint32_t>(clipMax.y - clipMin.y);
            const auto scissor = VkRect2D{
                .offset = {scissorX, scissorY},
                .extent = {scissorWidth, scissorHeight},
            };
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            const auto pcs = PushConstants{
                .vertexBuffer = vtxBuffer.getBuffer().address,
                .textureId = (std::uint32_t)textureId,
                .textureIsSRGB = textureIsSRGB,
                .translate = translate,
                .scale = scale,
            };
            vkCmdPushConstants(
                cmd,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(PushConstants),
                &pcs);

            vkCmdDrawIndexed(
                cmd,
                imCmd.ElemCount,
                1,
                imCmd.IdxOffset + globalIdxOffset,
                imCmd.VtxOffset + imCmd.VtxOffset + globalVtxOffset,
                0);
        }

        globalIdxOffset += cmdList.IdxBuffer.Size;
        globalVtxOffset += cmdList.VtxBuffer.Size;
    }

    vkCmdEndRendering(cmd);
}

void VulkanImGuiBackend::copyBuffers(VkCommandBuffer cmd, GfxDevice& gfxDevice) const
{
    const auto* drawData = ImGui::GetDrawData();

    {
        // sync with previous read
        const auto idxBufferBarrier = VkBufferMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .buffer = idxBuffer.getBuffer().buffer,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        };
        const auto vtxBufferBarrier = VkBufferMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .buffer = vtxBuffer.getBuffer().buffer,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        };
        const std::array<VkBufferMemoryBarrier2, 2> barriers{idxBufferBarrier, vtxBufferBarrier};
        const auto dependencyInfo = VkDependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = 2,
            .pBufferMemoryBarriers = barriers.data(),
        };
        vkCmdPipelineBarrier2(cmd, &dependencyInfo);
    }

    const auto currFrameIndex = gfxDevice.getCurrentFrameIndex();
    std::size_t currentIndexOffset = 0;
    std::size_t currentVertexOffset = 0;
    for (int i = 0; i < drawData->CmdListsCount; ++i) {
        const auto& cmdList = *drawData->CmdLists[i];
        const auto* idxData = reinterpret_cast<const ImDrawIdx*>(cmdList.IdxBuffer.Data);
        const auto* vtxData = reinterpret_cast<const ImDrawVert*>(cmdList.VtxBuffer.Data);
        idxBuffer.uploadNewData(
            cmd,
            currFrameIndex,
            (void*)cmdList.IdxBuffer.Data,
            sizeof(ImDrawIdx) * cmdList.IdxBuffer.Size,
            sizeof(ImDrawIdx) * currentIndexOffset,
            false);
        vtxBuffer.uploadNewData(
            cmd,
            currFrameIndex,
            (void*)cmdList.VtxBuffer.Data,
            sizeof(ImDrawVert) * cmdList.VtxBuffer.Size,
            sizeof(ImDrawVert) * currentVertexOffset,
            false);

        currentIndexOffset += cmdList.IdxBuffer.Size;
        currentVertexOffset += cmdList.VtxBuffer.Size;
    }

    { // sync write
        const auto idxBufferBarrier = VkBufferMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
            .buffer = idxBuffer.getBuffer().buffer,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        };
        const auto vtxBufferBarrier = VkBufferMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
            .buffer = vtxBuffer.getBuffer().buffer,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        };
        const std::array<VkBufferMemoryBarrier2, 2> barriers{idxBufferBarrier, vtxBufferBarrier};
        const auto dependencyInfo = VkDependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = 2,
            .pBufferMemoryBarriers = barriers.data(),
        };
        vkCmdPipelineBarrier2(cmd, &dependencyInfo);
    }
}

void VulkanImGuiBackend::cleanup(GfxDevice& gfxDevice)
{
    vkDestroyPipelineLayout(gfxDevice.getDevice(), pipelineLayout, nullptr);
    vkDestroyPipeline(gfxDevice.getDevice(), pipeline, nullptr);

    idxBuffer.cleanup(gfxDevice);
    vtxBuffer.cleanup(gfxDevice);
}
