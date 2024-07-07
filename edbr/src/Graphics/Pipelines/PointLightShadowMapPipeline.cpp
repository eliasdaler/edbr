#include <edbr/Graphics/Pipelines/PointLightShadowMapPipeline.h>

#include <edbr/Graphics/FrustumCulling.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/Light.h>
#include <edbr/Graphics/MeshCache.h>
#include <edbr/Graphics/MeshDrawCommand.h>
#include <edbr/Graphics/ShadowMapping.h>
#include <edbr/Graphics/Vulkan/Pipelines.h>
#include <edbr/Graphics/Vulkan/Util.h>

void PointLightShadowMapPipeline::init(GfxDevice& gfxDevice, float pointLightMaxRange)
{
    assert(pointLightMaxRange > 0.f);
    this->pointLightMaxRange = pointLightMaxRange;

    const auto& device = gfxDevice.getDevice();
    const auto vertexShader = vkutil::loadShaderModule("shaders/shadow_map_point.vert.spv", device);
    const auto fragShader = vkutil::loadShaderModule("shaders/shadow_map_point.frag.spv", device);

    vkutil::addDebugLabel(device, vertexShader, "shadow_map_point.vert");
    vkutil::addDebugLabel(device, fragShader, "shadow_map_point.frag");

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    const auto pushConstantRanges = std::array{bufferRange};
    const auto layouts = std::array{gfxDevice.getBindlessDescSetLayout()};
    pipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);

    pipeline = PipelineBuilder{pipelineLayout}
                   .setShaders(vertexShader, fragShader)
                   .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                   .setPolygonMode(VK_POLYGON_MODE_FILL)
                   .enableCulling()
                   .setMultisamplingNone()
                   .disableBlending()
                   .setDepthFormat(VK_FORMAT_D32_SFLOAT)
                   .enableDepthClamp()
                   .enableDepthTest(true, VK_COMPARE_OP_LESS)
                   .build(device);
    vkutil::addDebugLabel(device, pipeline, "shadow map (point) pipeline");

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);

    shadowMapImageViews.resize(6 * MAX_POINT_LIGHTS);
    for (std::size_t j = 0; j < MAX_POINT_LIGHTS; ++j) {
        shadowMaps[j] = gfxDevice.createImage(
            {
                .format = VK_FORMAT_D32_SFLOAT,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
                .extent =
                    VkExtent3D{
                        (std::uint32_t)shadowMapTextureSize,
                        (std::uint32_t)shadowMapTextureSize,
                        1},
                .numLayers = 6 * MAX_POINT_LIGHTS,
                .isCubemap = true,
            },
            fmt::format("point light shadow map[{}]", j).c_str());

        // create shadow map image views
        const auto& shadowMap = gfxDevice.getImage(shadowMaps[j]);
        for (std::size_t i = 0; i < 6; ++i) {
            const auto viewCreateInfo = VkImageViewCreateInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = shadowMap.image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = VK_FORMAT_D32_SFLOAT,
                .subresourceRange =
                    VkImageSubresourceRange{
                        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = (std::uint32_t)i,
                        .layerCount = 1,
                    },
            };

            VK_CHECK(vkCreateImageView(
                gfxDevice.getDevice(), &viewCreateInfo, nullptr, &shadowMapImageViews[i + j * 6]));
        }
    }

    { // init point light shadow map cameras
        const auto aspect = 1.f; // shadow maps are square
        const auto nearPlane = 0.1f;
        const auto farPlane = pointLightMaxRange;

        // <front, up>
        static const std::array<std::pair<glm::vec3, glm::vec3>, 6> shadowDirections{{
            {{-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}}, // posx
            {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}}, // negx
            {{0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}}, // posy
            {{0.0, 1.0, 0.0}, {0.0, 0.0, -1.0}}, // negy
            {{0.0, 0.0, 1.0}, {0.0, 1.0, 0.0}}, // posx
            {{0.0, 0.0, -1.0}, {0.0, 1.0, 0.0}}, // negz
        }};

        for (std::size_t j = 0; j < MAX_POINT_LIGHTS; ++j) {
            for (std::size_t i = 0; i < 6; ++i) {
                auto& camera = pointLightShadowMapCameras[i + j * 6];
                camera.setHeading(
                    glm::quatLookAt(shadowDirections[i].first, shadowDirections[i].second));
                camera.init(glm::radians(90.0f), nearPlane, farPlane, aspect);
            }
        }
    }

    vpsBuffer.init(
        gfxDevice,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        sizeof(glm::mat4) * 6 * MAX_POINT_LIGHTS,
        graphics::FRAME_OVERLAP,
        "point light vps data");
}

void PointLightShadowMapPipeline::cleanup(GfxDevice& gfxDevice)
{
    vpsBuffer.cleanup(gfxDevice);

    for (const auto& view : shadowMapImageViews) {
        vkDestroyImageView(gfxDevice.getDevice(), view, nullptr);
    }

    vkDestroyPipelineLayout(gfxDevice.getDevice(), pipelineLayout, nullptr);
    vkDestroyPipeline(gfxDevice.getDevice(), pipeline, nullptr);
}

void PointLightShadowMapPipeline::beginFrame(
    VkCommandBuffer cmd,
    const GfxDevice& gfxDevice,
    const std::vector<GPULightData>& lightData,
    const std::vector<std::size_t> pointLightIndices)
{
    lightToShadowMapId.clear();
    currShadowMapIndex = 0;

    // calculate shadow map vps
    auto currIndex = 0;
    for (const auto lightIndex : pointLightIndices) {
        for (std::size_t i = 0; i < 6; ++i) {
            auto& currCamera = pointLightShadowMapCameras[i + currIndex * 6];
            currCamera.setPosition(lightData[lightIndex].position);
            pointLightShadowMapVPs[i + currIndex * 6] = currCamera.getViewProj();
        }
        ++currIndex;
    }
    // upload to GPU
    vpsBuffer.uploadNewData(
        cmd,
        gfxDevice.getCurrentFrameIndex(),
        (void*)pointLightShadowMapVPs.data(),
        sizeof(pointLightShadowMapVPs));

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    gfxDevice.bindBindlessDescSet(cmd, pipelineLayout);
}

void PointLightShadowMapPipeline::draw(
    VkCommandBuffer cmd,
    const GfxDevice& gfxDevice,
    const MeshCache& meshCache,
    const Camera& camera,
    const std::uint32_t lightIndex,
    const glm::vec3& lightPos,
    const GPUBuffer& materialsBuffer,
    const GPUBuffer& lightsBuffer,
    const std::vector<MeshDrawCommand>& meshDrawCommands,
    bool shadowsEnabled)
{
    if (currShadowMapIndex >= MAX_POINT_LIGHTS) {
        fmt::println(
            "too many point lights with shadows: the shadow won't be drawn for the current light");
        return;
    }

    const auto& shadowMap = gfxDevice.getImage(shadowMaps[currShadowMapIndex]);
    vkutil::transitionImage(
        cmd, shadowMap.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    for (std::size_t i = 0; i < 6; ++i) {
        const auto& currCamera = pointLightShadowMapCameras[i + currShadowMapIndex * 6];

        const auto renderInfo = vkutil::createRenderingInfo({
            .renderExtent =
                {(std::uint32_t)shadowMapTextureSize, (std::uint32_t)shadowMapTextureSize},
            .depthImageView = shadowMapImageViews[i + currShadowMapIndex * 6],
            .depthImageClearValue = 1.f,
        });
        vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

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

        const auto frustum = edge::createFrustumFromCamera(currCamera);
        auto prevMeshId = NULL_MESH_ID;

        for (const auto& dc : meshDrawCommands) {
            if (!shadowsEnabled || !dc.castShadow) {
                continue;
            }

            if (!edge::isInFrustum(frustum, dc.worldBoundingSphere)) {
                // hack: don't cull big objects, because shadows from them might disappear
                if (dc.worldBoundingSphere.radius < 2.f) {
                    continue;
                }
            }

            const auto& mesh = meshCache.getMesh(dc.meshId);

            if (dc.meshId != prevMeshId) {
                prevMeshId = dc.meshId;
                vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            }

            const auto pushConstants = PushConstants{
                .model = dc.transformMatrix,
                .vertexBuffer = dc.skinnedMesh ? dc.skinnedMesh->skinnedVertexBuffer.address :
                                                 mesh.vertexBuffer.address,
                .materialsBuffer = materialsBuffer.address,
                .lightsBuffer = lightsBuffer.address,
                .vpsBuffer = vpsBuffer.getBuffer().address,
                .materialId = dc.materialId,
                .lightIndex = lightIndex,
                .vpsBufferIndex = (std::uint32_t)(i + currShadowMapIndex * 6),
                .farPlane = pointLightMaxRange,
            };
            vkCmdPushConstants(
                cmd,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(PushConstants),
                &pushConstants);

            vkCmdDrawIndexed(cmd, mesh.numIndices, 1, 0, 0, 0);
        }

        vkCmdEndRendering(cmd);
    }

    lightToShadowMapId[lightIndex] = shadowMaps[currShadowMapIndex];

    vkutil::transitionImage(
        cmd,
        shadowMap.image,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);

    ++currShadowMapIndex;
}

void PointLightShadowMapPipeline::endFrame(VkCommandBuffer cmd, const GfxDevice& gfxDevice)
{}
