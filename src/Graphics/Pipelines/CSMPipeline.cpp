#include "CSMPipeline.h"

#include <Graphics/Vulkan/Init.h>
#include <Graphics/Vulkan/Pipelines.h>
#include <Graphics/Vulkan/Util.h>

#include <Graphics/DrawCommand.h>
#include <Graphics/FrustumCulling.h>
#include <Graphics/Renderer.h>
#include <Graphics/ShadowMapping.h>

CSMPipeline::CSMPipeline(
    Renderer& renderer,
    const std::array<float, NUM_SHADOW_CASCADES>& percents) :
    percents(percents), renderer(renderer)
{
    const auto& device = renderer.getDevice();
    const auto vertexShader = vkutil::loadShaderModule("shaders/mesh_depth_only.vert.spv", device);

    vkutil::addDebugLabel(device, vertexShader, "mesh_depth_only.vert");

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(DepthOnlyPushConstants),
    };

    const auto pushConstantRanges = std::array{bufferRange};
    const auto layouts =
        std::array{renderer.sceneDataDescriptorLayout, renderer.meshMaterialLayout};
    meshDepthOnlyPipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);

    meshDepthOnlyPipeline = PipelineBuilder{meshDepthOnlyPipelineLayout}
                                .setShaders(vertexShader, VK_NULL_HANDLE)
                                .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                .setPolygonMode(VK_POLYGON_MODE_FILL)
                                .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
                                .setMultisamplingNone()
                                .disableBlending()
                                .setDepthFormat(VK_FORMAT_D32_SFLOAT)
                                .enableDepthClamp()
                                .enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                                .build(device);
    vkutil::addDebugLabel(device, meshDepthOnlyPipeline, "mesh depth only pipeline");

    vkDestroyShaderModule(device, vertexShader, nullptr);

    initCSMData();
}

void CSMPipeline::initCSMData()
{
    csmShadowMap = renderer.createImage({
        .format = VK_FORMAT_D32_SFLOAT,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .extent =
            VkExtent3D{(std::uint32_t)shadowMapTextureSize, (std::uint32_t)shadowMapTextureSize, 1},
        .numLayers = NUM_SHADOW_CASCADES,
    });
    vkutil::addDebugLabel(renderer.getDevice(), csmShadowMap.image, "CSM shadow map");
    vkutil::addDebugLabel(renderer.getDevice(), csmShadowMap.imageView, "CSM shadow map view");

    for (int i = 0; i < NUM_SHADOW_CASCADES; ++i) {
        const auto createInfo = VkImageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = csmShadowMap.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = csmShadowMap.format,
            .subresourceRange =
                VkImageSubresourceRange{
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = (std::uint32_t)i,
                    .layerCount = 1,
                },
        };
        VK_CHECK(
            vkCreateImageView(renderer.getDevice(), &createInfo, nullptr, &csmShadowMapViews[i]));
        vkutil::addDebugLabel(renderer.getDevice(), csmShadowMapViews[i], "CSM shadow map view");
    }
}

void CSMPipeline::cleanup(VkDevice device)
{
    vkDestroyPipelineLayout(device, meshDepthOnlyPipelineLayout, nullptr);
    vkDestroyPipeline(device, meshDepthOnlyPipeline, nullptr);
    renderer.destroyImage(csmShadowMap);
    for (int i = 0; i < NUM_SHADOW_CASCADES; ++i) {
        vkDestroyImageView(device, csmShadowMapViews[i], nullptr);
    }
}

void CSMPipeline::draw(
    VkCommandBuffer cmd,
    const Camera& camera,
    const glm::vec3& sunlightDirection,
    const std::vector<DrawCommand>& drawCommands)
{
    vkutil::transitionImage(
        cmd,
        csmShadowMap.image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    /* std::array<float, NUM_SHADOW_CASCADES> percents = {0.3f, 0.8f, 1.f};
    if (camera.getZFar() > 500.f) {
        percents = {0.005f, 0.01f, 0.15f};
    } */

    for (std::size_t i = 0; i < NUM_SHADOW_CASCADES; ++i) {
        float zNear = i == 0 ? camera.getZNear() : camera.getZNear() * percents[i - 1];
        float zFar = camera.getZFar() * percents[i];
        cascadeFarPlaneZs[i] = zFar;

        // create subfustrum by copying everything about the main camera,
        // but changing zFar
        Camera subFrustumCamera;
        subFrustumCamera.setPosition(camera.getPosition());
        subFrustumCamera.setHeading(camera.getHeading());
        subFrustumCamera.init(camera.getFOVX(), zNear, zFar, 1.f);
        // NOTE: this camera doesn't use inverse depth because otherwise
        // calculateFrustumCornersWorldSpace doesn't work properly

        const auto corners =
            edge::calculateFrustumCornersWorldSpace(subFrustumCamera.getViewProj());
        const auto csmCamera = calculateCSMCamera(corners, sunlightDirection, shadowMapTextureSize);
        csmLightSpaceTMs[i] = csmCamera.getViewProj();

        const auto renderInfo = vkutil::createRenderingInfo({
            .renderExtent =
                {(std::uint32_t)shadowMapTextureSize, (std::uint32_t)shadowMapTextureSize},
            .depthImageView = csmShadowMapViews[i],
            .depthImageClearValue = 0.f,
        });
        vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshDepthOnlyPipeline);

        const auto viewport = VkViewport{
            .x = 0,
            .y = 0,
            .width = shadowMapTextureSize,
            .height = shadowMapTextureSize,
            .minDepth = 0.f,
            .maxDepth = 1.f,
        };
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        const auto scissor = VkRect2D{
            .offset = {},
            .extent = {(std::uint32_t)shadowMapTextureSize, (std::uint32_t)shadowMapTextureSize},
        };
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        const auto frustum = edge::createFrustumFromCamera(csmCamera);
        auto prevMeshId = NULL_MESH_ID;

        // FIXME: sort by mesh?
        for (const auto& dc : drawCommands) {
            // hack: don't cull big objects, because shadows from them might disappear
            if (dc.worldBoundingSphere.radius < 2.f &&
                !edge::isInFrustum(frustum, dc.worldBoundingSphere)) {
                continue;
            }

            const auto& mesh = renderer.getMesh(dc.meshId);

            if (dc.meshId != prevMeshId) {
                prevMeshId = dc.meshId;
                vkCmdBindIndexBuffer(cmd, mesh.buffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            }

            const auto pushConstants = DepthOnlyPushConstants{
                .mvp = csmLightSpaceTMs[i] * dc.transformMatrix,
                .vertexBuffer = dc.skinnedMesh ? dc.skinnedMesh->skinnedVertexBufferAddress :
                                                 mesh.buffers.vertexBufferAddress,
            };
            vkCmdPushConstants(
                cmd,
                meshDepthOnlyPipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(DepthOnlyPushConstants),
                &pushConstants);

            vkCmdDrawIndexed(cmd, mesh.numIndices, 1, 0, 0, 0);
        }

        vkCmdEndRendering(cmd);
    }
}
