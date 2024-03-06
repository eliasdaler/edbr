#include "GameRenderer.h"

#include <Graphics/Cubemap.h>
#include <Graphics/FrustumCulling.h>
#include <Graphics/Scene.h>
#include <Graphics/Vulkan/Init.h>
#include <Graphics/Vulkan/Pipelines.h>
#include <Graphics/Vulkan/Util.h>
#include <util/GltfLoader.h>

#include <imgui.h>

#include <numeric> // iota

void GameRenderer::init(SDL_Window* window, bool vSync)
{
    renderer.init(window, vSync);
    samples = renderer.getHighestSupportedSamplingCount();

    createDrawImage(renderer.getSwapchainExtent(), true);

    skinningPipeline = std::make_unique<SkinningPipeline>(renderer);
    csmPipeline = std::make_unique<CSMPipeline>(renderer, std::array{0.08f, 0.2f, 0.5f});
    meshPipeline =
        std::make_unique<MeshPipeline>(renderer, drawImage.format, depthImage.format, samples);
    skyboxPipeline =
        std::make_unique<SkyboxPipeline>(renderer, drawImage.format, depthImage.format, samples);
    depthResolvePipeline = std::make_unique<DepthResolvePipeline>(renderer, depthImage);
    postFXPipeline = std::make_unique<PostFXPipeline>(renderer, drawImage.format);

    skyboxImage = graphics::loadCubemap(renderer, "assets/images/skybox/distant_sunset");
    skyboxPipeline->setSkyboxImage(skyboxImage);
}

void GameRenderer::createDrawImage(VkExtent2D extent, bool firstCreate)
{
    const auto drawImageExtent = VkExtent3D{
        .width = extent.width,
        .height = extent.height,
        .depth = 1,
    };

    { // setup draw image
        VkImageUsageFlags usages{};
        usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        usages |= VK_IMAGE_USAGE_SAMPLED_BIT;

        auto createImageInfo = vkutil::CreateImageInfo{
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = usages,
            .extent = drawImageExtent,
            .samples = samples,
        };
        drawImage = renderer.createImage(createImageInfo);

        vkutil::addDebugLabel(renderer.getDevice(), drawImage.image, "draw image");
        vkutil::addDebugLabel(renderer.getDevice(), drawImage.imageView, "draw image view");

        if (firstCreate) {
            createImageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // no MSAA
            postFXDrawImage = renderer.createImage(createImageInfo);
            vkutil::
                addDebugLabel(renderer.getDevice(), postFXDrawImage.image, "post FX draw image");
            vkutil::addDebugLabel(
                renderer.getDevice(), postFXDrawImage.imageView, "post FX draw image view");
        }
    }

    if (firstCreate) { // setup resolve image
        VkImageUsageFlags usages{};
        usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        usages |= VK_IMAGE_USAGE_SAMPLED_BIT;

        const auto createImageInfo = vkutil::CreateImageInfo{
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = usages,
            .extent = drawImageExtent,
        };
        resolveImage = renderer.createImage(createImageInfo);

        vkutil::addDebugLabel(renderer.getDevice(), resolveImage.image, "resolve image");
        vkutil::addDebugLabel(renderer.getDevice(), resolveImage.imageView, "resolve image");
    }

    { // setup depth image
        auto createInfo = vkutil::CreateImageInfo{
            .format = VK_FORMAT_D32_SFLOAT,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .extent = drawImageExtent,
            .samples = samples,
        };

        depthImage = renderer.createImage(createInfo);
        vkutil::addDebugLabel(renderer.getDevice(), depthImage.image, "draw image (depth)");
        vkutil::addDebugLabel(renderer.getDevice(), depthImage.imageView, "draw image depth view");

        if (firstCreate) {
            createInfo.samples = VK_SAMPLE_COUNT_1_BIT; // NO MSAA
            resolveDepthImage = renderer.createImage(createInfo);

            vkutil::addDebugLabel(
                renderer.getDevice(), resolveDepthImage.image, "draw image (depth resolve)");
            vkutil::addDebugLabel(
                renderer.getDevice(), resolveDepthImage.imageView, "draw image depth resolve view");
        }
    }
}

void GameRenderer::draw(const Camera& camera, const RendererSceneData& sceneData)
{
    auto cmd = renderer.beginFrame();
    draw(cmd, camera, sceneData);

    // we'll copy from draw image to swapchain
    vkutil::transitionImage(
        cmd,
        postFXDrawImage.image,
        VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    renderer.endFrame(cmd, postFXDrawImage);
}

void GameRenderer::draw(
    VkCommandBuffer cmd,
    const Camera& camera,
    const RendererSceneData& sceneData)
{
    { // skinning
        vkutil::cmdBeginLabel(cmd, "Skinning");
        for (const auto& dc : drawCommands) {
            if (!dc.skinnedMesh) {
                continue;
            }
            skinningPipeline->doSkinning(cmd, dc);
        }
        vkutil::cmdEndLabel(cmd);

        // Sync skinning with CSM
        const auto memoryBarrier = VkMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT_KHR,
            .dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT_KHR,
        };
        const auto dependencyInfo = VkDependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .memoryBarrierCount = 1,
            .pMemoryBarriers = &memoryBarrier,
        };
        vkCmdPipelineBarrier2(cmd, &dependencyInfo);
    }

    { // CSM
        TracyVkZoneC(
            renderer.getCurrentFrame().tracyVkCtx, cmd, "CSM", tracy::Color::CornflowerBlue);
        vkutil::cmdBeginLabel(cmd, "CSM");

        vkutil::transitionImage(
            cmd,
            csmPipeline->getShadowMap().image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        csmPipeline->draw(cmd, camera, glm::vec3{sceneData.sunlightDirection}, drawCommands);

        vkutil::cmdEndLabel(cmd);

        // Sync CSM with geometry pass
        const auto imageBarrier = VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,
            .image = csmPipeline->getShadowMap().image,
            .subresourceRange = vkinit::imageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT),
        };
        const auto dependencyInfo = VkDependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &imageBarrier,
        };
        vkCmdPipelineBarrier2(cmd, &dependencyInfo);
    }

    { // Geometry
        TracyVkZoneC(
            renderer.getCurrentFrame().tracyVkCtx, cmd, "Geometry", tracy::Color::ForestGreen);
        vkutil::cmdBeginLabel(cmd, "Geometry");

        // upload scene data
        const auto gpuSceneData = GPUSceneData{
            .view = sceneData.camera.getView(),
            .proj = sceneData.camera.getProjection(),
            .viewProj = sceneData.camera.getViewProj(),
            .cameraPos = glm::vec4{sceneData.camera.getPosition(), 1.f},
            .ambientColorAndIntensity = sceneData.ambientColorAndIntensity,
            .sunlightDirection = sceneData.sunlightDirection,
            .sunlightColorAndIntensity = sceneData.sunlightColorAndIntensity,
            .cascadeFarPlaneZs =
                glm::vec4{
                    csmPipeline->cascadeFarPlaneZs[0],
                    csmPipeline->cascadeFarPlaneZs[1],
                    csmPipeline->cascadeFarPlaneZs[2],
                    0.f,
                },
            .csmLightSpaceTMs = csmPipeline->csmLightSpaceTMs,
        };
        const auto sceneDataDesctiptor =
            renderer.uploadSceneData(gpuSceneData, csmPipeline->getShadowMap());

        vkutil::transitionImage(
            cmd,
            drawImage.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        vkutil::transitionImage(
            cmd,
            depthImage.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        if (isMultisamplingEnabled()) {
            vkutil::transitionImage(
                cmd,
                resolveImage.image,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }

        const auto renderInfo = vkutil::createRenderingInfo({
            .renderExtent = drawImage.getExtent2D(),
            .colorImageView = drawImage.imageView,
            .colorImageClearValue = glm::vec4{0.f, 0.f, 0.f, 0.f},
            .depthImageView = depthImage.imageView,
            .depthImageClearValue = 0.f,
            .resolveImageView = isMultisamplingEnabled() ? resolveImage.imageView : VK_NULL_HANDLE,
        });

        vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

        meshPipeline->draw(
            cmd,
            drawImage.getExtent2D(),
            camera,
            sceneDataDesctiptor,
            drawCommands,
            sortedDrawCommands);

        // sky
        skyboxPipeline->draw(cmd, camera);

        vkCmdEndRendering(cmd);
        vkutil::cmdEndLabel(cmd); // geometry
    }

    { // Sync Geometry/Sky with next pass
        const auto imageBarrier = VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .image = isMultisamplingEnabled() ? resolveImage.image : drawImage.image,
            .subresourceRange = vkinit::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT),
        };
        const auto depthBarrier = VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .image = depthImage.image,
            .subresourceRange = vkinit::imageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT),
        };
        const auto barriers = std::array{imageBarrier, depthBarrier};
        const auto dependencyInfo = VkDependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = barriers.size(),
            .pImageMemoryBarriers = barriers.data(),
        };
        vkCmdPipelineBarrier2(cmd, &dependencyInfo);
    }

    if (isMultisamplingEnabled()) {
        TracyVkZoneC(
            renderer.getCurrentFrame().tracyVkCtx, cmd, "Depth resolve", tracy::Color::ForestGreen);
        vkutil::cmdBeginLabel(cmd, "Depth resolve");

        vkutil::transitionImage(
            cmd,
            resolveDepthImage.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        const auto renderInfo = vkutil::createRenderingInfo({
            .renderExtent = resolveDepthImage.getExtent2D(),
            .depthImageView = resolveDepthImage.imageView,
        });

        vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

        depthResolvePipeline
            ->draw(cmd, depthImage.getExtent2D(), vkutil::sampleCountToInt(samples));

        vkCmdEndRendering(cmd);

        // sync
        vkutil::transitionImage(
            cmd,
            resolveDepthImage.image,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkutil::cmdEndLabel(cmd);
    }

    // Now we'll draw into another draw image and use currDrawImage and
    // depthImage as attachments
    vkutil::transitionImage(
        cmd,
        postFXDrawImage.image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    if (isMultisamplingEnabled()) {
        postFXPipeline->setImages(resolveImage, resolveDepthImage);
    } else {
        postFXPipeline->setImages(drawImage, depthImage);
    }

    { // post FX
        vkutil::cmdBeginLabel(cmd, "Post FX");

        const auto renderInfo = vkutil::createRenderingInfo({
            .renderExtent = postFXDrawImage.getExtent2D(),
            .colorImageView = postFXDrawImage.imageView,
        });

        vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);
        const auto pcs = PostFXPipeline::PostFXPushContants{
            .invProj = glm::inverse(camera.getProjection()),
            .fogColorAndDensity = sceneData.fogColorAndDensity,
            .ambientColorAndIntensity = sceneData.ambientColorAndIntensity,
            .sunlightColorAndIntensity = sceneData.sunlightColorAndIntensity,
        };
        postFXPipeline->draw(cmd, pcs);
        vkCmdEndRendering(cmd);

        vkutil::cmdEndLabel(cmd);
    }
}

void GameRenderer::cleanup()
{
    const auto& device = renderer.getDevice();

    vkDeviceWaitIdle(device);

    postFXPipeline->cleanup(device);
    depthResolvePipeline->cleanup(device);
    skyboxPipeline->cleanup(device);
    meshPipeline->cleanup(device);
    csmPipeline->cleanup(device);
    skinningPipeline->cleanup(device);

    renderer.destroyImage(skyboxImage);

    renderer.destroyImage(resolveDepthImage);
    renderer.destroyImage(depthImage);
    renderer.destroyImage(resolveImage);
    renderer.destroyImage(postFXDrawImage);
    renderer.destroyImage(drawImage);

    renderer.cleanup();
}

void GameRenderer::updateDevTools(float dt)
{
    renderer.updateDevTools(dt);
    ImGui::DragFloat3("Cascades", csmPipeline->percents.data(), 0.1f, 0.f, 1.f);

    if (ImGui::BeginCombo("MSAA", vkutil::sampleCountToString(samples))) {
        static const auto counts = std::array{
            VK_SAMPLE_COUNT_1_BIT,
            VK_SAMPLE_COUNT_2_BIT,
            VK_SAMPLE_COUNT_4_BIT,
            VK_SAMPLE_COUNT_8_BIT,
            VK_SAMPLE_COUNT_16_BIT,
            VK_SAMPLE_COUNT_32_BIT,
            VK_SAMPLE_COUNT_64_BIT,
        };
        for (const auto& count : counts) {
            if (!renderer.deviceSupportsSamplingCount(count)) {
                continue;
            }
            bool isSelected = (count == samples);
            if (ImGui::Selectable(vkutil::sampleCountToString(count), isSelected)) {
                samples = count;
                onMultisamplingStateUpdate();
            }
        }
        ImGui::EndCombo();
    }
}

bool GameRenderer::isMultisamplingEnabled() const
{
    return samples != VK_SAMPLE_COUNT_1_BIT;
}

void GameRenderer::onMultisamplingStateUpdate()
{
    vkDeviceWaitIdle(renderer.getDevice());

    { // cleanup old state
        meshPipeline->cleanup(renderer.getDevice());
        skyboxPipeline->cleanup(renderer.getDevice());

        renderer.destroyImage(depthImage);
        renderer.destroyImage(drawImage);
    }

    meshPipeline =
        std::make_unique<MeshPipeline>(renderer, drawImage.format, depthImage.format, samples);
    skyboxPipeline =
        std::make_unique<SkyboxPipeline>(renderer, drawImage.format, depthImage.format, samples);
    skyboxPipeline->setSkyboxImage(skyboxImage);

    createDrawImage(renderer.getSwapchainExtent(), false);

    if (isMultisamplingEnabled()) {
        depthResolvePipeline->setDepthImage(renderer, depthImage);
    }
}

Scene GameRenderer::loadScene(const std::filesystem::path& path)
{
    util::LoadContext loadContext{
        .renderer = renderer,
    };
    util::SceneLoader loader;

    Scene scene;
    loader.loadScene(loadContext, scene, path);
    return scene;
}

void GameRenderer::beginDrawing()
{
    drawCommands.clear();
    skinningPipeline->beginDrawing();
}

void GameRenderer::endDrawing()
{
    sortDrawList();
}

void GameRenderer::addDrawCommand(MeshId id, const glm::mat4& transform)
{
    const auto& mesh = renderer.getMesh(id);
    const auto worldBoundingSphere =
        edge::calculateBoundingSphereWorld(transform, mesh.boundingSphere, false);

    drawCommands.push_back(DrawCommand{
        .meshId = id,
        .transformMatrix = transform,
        .worldBoundingSphere = worldBoundingSphere,
    });
}

void GameRenderer::addDrawSkinnedMeshCommand(
    std::span<const MeshId> meshes,
    std::span<const SkinnedMesh> skinnedMeshes,
    const glm::mat4& transform,
    std::span<const glm::mat4> jointMatrices)
{
    const auto startIndex = skinningPipeline->appendJointMatrices(jointMatrices);

    assert(meshes.size() == skinnedMeshes.size());
    for (std::size_t i = 0; i < meshes.size(); ++i) {
        const auto& mesh = renderer.getMesh(meshes[i]);
        assert(mesh.hasSkeleton);

        const auto worldBoundingSphere =
            edge::calculateBoundingSphereWorld(transform, mesh.boundingSphere, true);
        drawCommands.push_back(DrawCommand{
            .meshId = meshes[i],
            .transformMatrix = transform,
            .worldBoundingSphere = worldBoundingSphere,
            .skinnedMesh = &skinnedMeshes[i],
            .jointMatricesStartIndex = (std::uint32_t)startIndex,
        });
    }
}

void GameRenderer::sortDrawList()
{
    sortedDrawCommands.clear();
    sortedDrawCommands.resize(drawCommands.size());
    std::iota(sortedDrawCommands.begin(), sortedDrawCommands.end(), 0);

    std::sort(
        sortedDrawCommands.begin(),
        sortedDrawCommands.end(),
        [this](const auto& i1, const auto& i2) {
            const auto& dc1 = drawCommands[i1];
            const auto& dc2 = drawCommands[i2];
            const auto& mesh1 = renderer.getMesh(dc1.meshId);
            const auto& mesh2 = renderer.getMesh(dc2.meshId);
            if (mesh1.materialId == mesh2.materialId) {
                return dc1.meshId < dc2.meshId;
            }
            return mesh1.materialId < mesh2.materialId;
        });
}
