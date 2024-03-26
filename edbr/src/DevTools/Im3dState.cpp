#include <edbr/DevTools/Im3dState.h>

#include <edbr/Graphics/CoordUtil.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/Vulkan/Pipelines.h>
#include <edbr/Graphics/Vulkan/Util.h>
#include <edbr/Math/GlobalAxes.h>
#include <edbr/Util/Im3dUtil.h>

#include <im3d.h>
#include <im3d_math.h>
#include <imgui.h>

namespace
{
static const int MAX_VTX_COUNT = 1000000;
}

// can't init here because Im3d might still have no context at this point
Im3d::Id Im3dState::DefaultLayer;
Im3d::Id Im3dState::WorldWithDepthLayer;
Im3d::Id Im3dState::WorldNoDepthLayer;

void Im3dState::init(GfxDevice& gfxDevice, VkFormat drawImageFormat)
{
    // note that Im3d will sort draw lists in the order that layers are defined in
    Im3dState::DefaultLayer = 0;
    Im3dState::WorldWithDepthLayer = Im3d::MakeId("WorldNoDepth");
    Im3dState::WorldNoDepthLayer = Im3d::MakeId("World");

    auto& ap = Im3d::GetAppData();
    ap.m_flipGizmoWhenBehind = false;

    const auto device = gfxDevice.getDevice();

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT |
                      VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };
    const auto layouts = std::array{gfxDevice.getBindlessDescSetLayout()};
    const auto pushConstantRanges = std::array{bufferRange};

    { // points
        const auto pointsVertShader =
            vkutil::loadShaderModule("shaders/im3d_points.vert.spv", device);
        const auto pointsFragShader =
            vkutil::loadShaderModule("shaders/im3d_points.frag.spv", device);

        pointsPipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);
        vkutil::addDebugLabel(device, pointsPipelineLayout, "im3d points pipeline layout");

        pointsPipeline = PipelineBuilder{pointsPipelineLayout}
                             .setShaders(pointsVertShader, pointsFragShader)
                             .setInputTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
                             .setPolygonMode(VK_POLYGON_MODE_FILL)
                             .disableCulling()
                             .setMultisamplingNone()
                             .enableBlending()
                             .setColorAttachmentFormat(drawImageFormat)
                             .disableDepthTest()
                             .build(device);
        vkutil::addDebugLabel(device, pointsPipeline, "im3d points pipeline");

        vkDestroyShaderModule(device, pointsVertShader, nullptr);
        vkDestroyShaderModule(device, pointsFragShader, nullptr);
    }

    { // lines
        const auto linesVertShader =
            vkutil::loadShaderModule("shaders/im3d_lines.vert.spv", device);
        const auto linesGeomShader =
            vkutil::loadShaderModule("shaders/im3d_lines.geom.spv", device);
        const auto linesFragShader =
            vkutil::loadShaderModule("shaders/im3d_lines.frag.spv", device);

        linesPipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);
        vkutil::addDebugLabel(device, linesPipelineLayout, "im3d lines pipeline layout");

        linesPipeline = PipelineBuilder{linesPipelineLayout}
                            .setShaders(linesVertShader, linesGeomShader, linesFragShader)
                            .setInputTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
                            .setPolygonMode(VK_POLYGON_MODE_FILL)
                            .disableCulling()
                            .setMultisamplingNone()
                            .enableBlending()
                            .setColorAttachmentFormat(drawImageFormat)
                            .disableDepthTest()
                            .build(device);
        vkutil::addDebugLabel(device, linesPipeline, "im3d lines pipeline");

        vkDestroyShaderModule(device, linesVertShader, nullptr);
        vkDestroyShaderModule(device, linesGeomShader, nullptr);
        vkDestroyShaderModule(device, linesFragShader, nullptr);
    }

    { // triangles
        const auto trianglesVertShader =
            vkutil::loadShaderModule("shaders/im3d_triangles.vert.spv", device);
        const auto trianglesFragShader =
            vkutil::loadShaderModule("shaders/im3d_triangles.frag.spv", device);

        trianglesPipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);
        vkutil::addDebugLabel(device, trianglesPipelineLayout, "im3d triangles pipeline layout");

        trianglesPipeline = PipelineBuilder{trianglesPipelineLayout}
                                .setShaders(trianglesVertShader, trianglesFragShader)
                                .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                .setPolygonMode(VK_POLYGON_MODE_FILL)
                                .disableCulling()
                                .setMultisamplingNone()
                                .enableBlending()
                                .setColorAttachmentFormat(drawImageFormat)
                                .disableDepthTest()
                                .build(device);
        vkutil::addDebugLabel(device, trianglesPipeline, "im3d triangles pipeline");

        vkDestroyShaderModule(device, trianglesVertShader, nullptr);
        vkDestroyShaderModule(device, trianglesFragShader, nullptr);
    }

    vtxBuffer.init(
        gfxDevice,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        sizeof(Im3d::VertexData) * MAX_VTX_COUNT,
        graphics::FRAME_OVERLAP,
        "im3d vertex buffer");
}

void Im3dState::cleanup(GfxDevice& device)
{
    vkDestroyPipelineLayout(device.getDevice(), pointsPipelineLayout, nullptr);
    vkDestroyPipeline(device.getDevice(), pointsPipeline, nullptr);

    vkDestroyPipelineLayout(device.getDevice(), linesPipelineLayout, nullptr);
    vkDestroyPipeline(device.getDevice(), linesPipeline, nullptr);

    vkDestroyPipelineLayout(device.getDevice(), trianglesPipelineLayout, nullptr);
    vkDestroyPipeline(device.getDevice(), trianglesPipeline, nullptr);

    vtxBuffer.cleanup(device);
}

void Im3dState::newFrame(
    float dt,
    const glm::vec2& vpSize,
    const Camera& camera,
    const glm::ivec2& mousePos,
    bool leftMousePressed)
{
    auto& ad = Im3d::GetAppData();
    ad.m_deltaTime = dt;
    ad.m_viewportSize = glm2im3d(vpSize);
    ad.m_projOrtho = camera.isOrthographic();
    ad.m_viewOrigin = glm2im3d(camera.getPosition());
    ad.m_viewDirection = glm2im3d(camera.getTransform().getLocalFront());
    ad.m_worldUp = glm2im3d(math::GlobalUpAxis);
    if (camera.isOrthographic()) {
        ad.m_projScaleY = 1.f;
    } else {
        ad.m_projScaleY = tanf(camera.getFOVX());
    }

    // to clip coords
    const auto ndc = edbr::util::fromWindowCoordsToNDC(mousePos, static_cast<glm::ivec2>(vpSize));
    const auto ray_clip = glm::vec4{ndc, 0.f, 1.f};

    // to eye coords
    auto ray_eye = glm::inverse(camera.getProjection()) * ray_clip;
    ray_eye = glm::vec4{glm::vec2(ray_eye), 0.f, 0.f};

    // to world coords
    auto ray_wor = glm::inverse(camera.getView()) * ray_eye;
    ray_wor = glm::normalize(ray_wor);

    glm::vec3 rayOrigin = camera.getPosition();
    glm::vec4 rayDirection = ray_wor;

    ad.m_cursorRayOrigin = glm2im3d(rayOrigin);
    ad.m_cursorRayDirection = glm2im3d(glm::vec3(rayDirection));

    ad.m_keyDown[Im3d::Mouse_Left /*Im3d::Action_Select*/] = leftMousePressed;

    Im3d::NewFrame();
}

void Im3dState::endFrame()
{
    Im3d::EndFrame();
}

void Im3dState::drawText(
    const glm::mat4& viewProj,
    const glm::ivec2& gameWindowPos,
    const glm::ivec2& gameWindowSize)
{
    drawTextDrawListsImGui(
        Im3d::GetTextDrawLists(),
        Im3d::GetTextDrawListCount(),
        viewProj,
        gameWindowPos,
        gameWindowSize);
}

void Im3dState::draw(
    VkCommandBuffer cmd,
    GfxDevice& gfxDevice,
    VkImageView swapchainImageView,
    VkExtent2D swapchainExtent)
{
    if (Im3d::GetDrawListCount() == 0) {
        return;
    }
    copyVertices(cmd, gfxDevice);

    const auto renderInfo = vkutil::createRenderingInfo({
        .renderExtent = swapchainExtent,
        .colorImageView = swapchainImageView,
    });
    vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

    const auto* drawLists = Im3d::GetDrawLists();
    std::size_t currentVertexOffset = 0;
    auto prevPrimType = Im3d::DrawPrimitive_Count;
    for (size_t i = 0, n = Im3d::GetDrawListCount(); i < n; ++i) {
        const Im3d::DrawList& drawList = drawLists[i];

        const auto& renderState = renderStates[drawList.m_layerId];

        const auto& viewProj = renderState.viewProj;

        VkPipelineLayout currPipelineLayout{VK_NULL_HANDLE};
        VkPipeline currPipeline{VK_NULL_HANDLE};

        switch (drawList.m_primType) {
        case Im3d::DrawPrimitive_Points:
            currPipelineLayout = pointsPipelineLayout;
            currPipeline = pointsPipeline;
            break;
        case Im3d::DrawPrimitive_Lines:
            currPipelineLayout = linesPipelineLayout;
            currPipeline = linesPipeline;
            break;
        case Im3d::DrawPrimitive_Triangles:
            currPipelineLayout = trianglesPipelineLayout;
            currPipeline = trianglesPipeline;
            break;
        default:
            assert(false);
            break;
        };

        if (drawList.m_primType != prevPrimType) {
            prevPrimType = drawList.m_primType;

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, currPipeline);
            gfxDevice.bindBindlessDescSet(cmd, currPipelineLayout);

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

            const auto scissor = VkRect2D{
                .offset = {},
                .extent = swapchainExtent,
            };
            vkCmdSetScissor(cmd, 0, 1, &scissor);
        }

        const auto& ad = Im3d::GetAppData();
        const auto viewport = glm::vec2{ad.m_viewportSize.x, ad.m_viewportSize.y};
        const auto pcs = PushConstants{
            .vertexBuffer = vtxBuffer.getBuffer().address,
            .viewProj = renderState.viewProj,
            .viewport = viewport,
        };
        vkCmdPushConstants(
            cmd,
            currPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT |
                VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(PushConstants),
            &pcs);

        vkCmdDraw(cmd, drawList.m_vertexCount, 1, currentVertexOffset, 0);
        currentVertexOffset += drawList.m_vertexCount;
    }

    vkCmdEndRendering(cmd);
}

void Im3dState::drawTextDrawListsImGui(
    const Im3d::TextDrawList _textDrawLists[],
    std::uint32_t _count,
    const glm::mat4& viewProj,
    const glm::ivec2& gameWindowPos,
    const glm::ivec2& gameWindowSize)
{
    // this code is mostly from im3d_example.cpp with some changes
    // Using ImGui here as a simple means of rendering text draw lists, however as with
    // primitives the application is free to draw text in any conceivable  manner.

    ImDrawList* imDrawList = ImGui::GetWindowDrawList();
    for (std::uint32_t i = 0; i < _count; ++i) {
        const Im3d::TextDrawList& textDrawList = Im3d::GetTextDrawLists()[i];

        for (std::uint32_t j = 0; j < textDrawList.m_textDataCount; ++j) {
            const Im3d::TextData& textData = textDrawList.m_textData[j];
            if (textData.m_positionSize.w == 0.0f || textData.m_color.getA() == 0.0f) {
                continue;
            }

            auto screen = edbr::util::fromWorldCoordsToScreenCoords(
                {
                    textData.m_positionSize.x,
                    textData.m_positionSize.y,
                    textData.m_positionSize.z,
                },
                viewProj,
                gameWindowSize);
            if (screen == edbr::util::OFFSCREEN_COORD) {
                continue;
            }

            // All text data is stored in a single buffer; each textData instance has an offset
            // into this buffer.
            const char* text = textDrawList.m_textBuffer + textData.m_textBufferOffset;

            // Calculate the final text size in pixels to apply alignment flags correctly.
            ImGui::SetWindowFontScale(textData.m_positionSize.w); // NB no CalcTextSize API
                                                                  // which takes a font/size
                                                                  // directly...
            auto textSize = ImGui::CalcTextSize(text, text + textData.m_textLength);
            ImGui::SetWindowFontScale(1.0f);

            // Generate a pixel offset based on text flags.
            // default to center
            auto textOffset = glm::vec2(-textSize.x * 0.5f, -textSize.y * 0.5f);
            if ((textData.m_flags & Im3d::TextFlags_AlignLeft) != 0) {
                textOffset.x = -textSize.x;
            } else if ((textData.m_flags & Im3d::TextFlags_AlignRight) != 0) {
                textOffset.x = 0.0f;
            }

            if ((textData.m_flags & Im3d::TextFlags_AlignTop) != 0) {
                textOffset.y = -textSize.y;
            } else if ((textData.m_flags & Im3d::TextFlags_AlignBottom) != 0) {
                textOffset.y = 0.0f;
            }

            // Add text to the window draw list.
            screen = screen + textOffset;
            screen.x = std::round(screen.x);
            screen.y = std::round(screen.y);
            imDrawList->AddText(
                nullptr,
                textData.m_positionSize.w * ImGui::GetFontSize(),
                ImVec2(gameWindowPos.x + screen.x, gameWindowPos.y + screen.y),
                textData.m_color.getABGR(),
                text,
                text + textData.m_textLength);
        }
    }
}

void Im3dState::copyVertices(VkCommandBuffer cmd, GfxDevice& gfxDevice)
{
    const auto currFrameIndex = gfxDevice.getCurrentFrameIndex();

    { // sync with previous read
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
        const auto dependencyInfo = VkDependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = 1,
            .pBufferMemoryBarriers = &vtxBufferBarrier,
        };
        vkCmdPipelineBarrier2(cmd, &dependencyInfo);
    }

    const auto* drawLists = Im3d::GetDrawLists();
    std::size_t currentVertexOffset = 0;
    for (size_t i = 0, n = Im3d::GetDrawListCount(); i < n; ++i) {
        const Im3d::DrawList& drawList = drawLists[i];
        vtxBuffer.uploadNewData(
            cmd,
            currFrameIndex,
            (void*)drawList.m_vertexData,
            sizeof(Im3d::VertexData) * drawList.m_vertexCount,
            sizeof(Im3d::VertexData) * currentVertexOffset,
            false);
        currentVertexOffset += drawList.m_vertexCount;
    }

    { // sync write
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
        const auto dependencyInfo = VkDependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = 1,
            .pBufferMemoryBarriers = &vtxBufferBarrier,
        };
        vkCmdPipelineBarrier2(cmd, &dependencyInfo);
    }
}
