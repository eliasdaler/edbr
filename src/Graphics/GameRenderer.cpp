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
    createDrawImage(renderer.getSwapchainExtent());

    skinningPipeline = std::make_unique<SkinningPipeline>(renderer);
    csmPipeline = std::make_unique<CSMPipeline>(renderer);
    meshPipeline = std::make_unique<MeshPipeline>(renderer, drawImage.format, depthImage.format);
    skyboxPipeline =
        std::make_unique<SkyboxPipeline>(renderer, drawImage.format, depthImage.format);

    skyboxImage = graphics::loadCubemap(renderer, "assets/images/skybox/distant_sunset");
    skyboxPipeline->setSkyboxImage(skyboxImage);
}

void GameRenderer::createDrawImage(VkExtent2D extent)
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
        usages |= VK_IMAGE_USAGE_STORAGE_BIT;
        usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        drawImage = renderer.createImage({
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = usages,
            .extent = drawImageExtent,
        });
        vkutil::addDebugLabel(renderer.getDevice(), drawImage.image, "main draw image");
        vkutil::addDebugLabel(renderer.getDevice(), drawImage.imageView, "main draw image view");
    }

    { // setup depth image
        const auto usages = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depthImage = renderer.createImage({
            .format = VK_FORMAT_D32_SFLOAT,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .extent = drawImageExtent,
        });
        vkutil::addDebugLabel(renderer.getDevice(), depthImage.image, "main draw image (depth)");
        vkutil::
            addDebugLabel(renderer.getDevice(), depthImage.imageView, "main draw image depth view");
    }
}

void GameRenderer::draw(const Camera& camera, const RendererSceneData& sceneData)
{
    renderer.draw(
        [this, &camera, &sceneData](VkCommandBuffer cmd) { draw(cmd, camera, sceneData); },
        drawImage);
}

void GameRenderer::draw(
    VkCommandBuffer cmd,
    const Camera& camera,
    const RendererSceneData& sceneData)
{
    const auto gpuSceneData = GPUSceneData{
        .view = sceneData.camera.getView(),
        .proj = sceneData.camera.getProjection(),
        .viewProj = sceneData.camera.getViewProj(),
        .cameraPos = glm::vec4{sceneData.camera.getPosition(), 1.f},
        .ambientColorAndIntensity = sceneData.ambientColorAndIntensity,
        .sunlightDirection = sceneData.sunlightDirection,
        .sunlightColorAndIntensity = sceneData.sunlightColorAndIntensity,
    };

    { // skinning
        vkutil::cmdBeginLabel(cmd, "Do skinning");
        for (const auto& dc : drawCommands) {
            if (!dc.skinnedMesh) {
                continue;
            }
            skinningPipeline->doSkinning(cmd, dc);
        }
        vkutil::cmdEndLabel(cmd);

        // sync skinning with later render passes
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
        vkutil::cmdBeginLabel(cmd, "Draw CSM");

        vkutil::transitionImage(
            cmd,
            csmPipeline->getShadowMap().image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        csmPipeline->draw(cmd, camera, glm::vec3{sceneData.sunlightDirection}, drawCommands);

        // sync
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

        vkutil::cmdEndLabel(cmd);
    }

    { // Geometry + Sky
        TracyVkZoneC(
            renderer.getCurrentFrame().tracyVkCtx, cmd, "Geometry", tracy::Color::ForestGreen);
        vkutil::cmdBeginLabel(cmd, "Draw geometry");

        // update scene data (add CSM info) and upload
        auto newSceneData = gpuSceneData;
        newSceneData.cascadeFarPlaneZs = glm::vec4{
            csmPipeline->cascadeFarPlaneZs[0],
            csmPipeline->cascadeFarPlaneZs[1],
            csmPipeline->cascadeFarPlaneZs[2],
            0.f,
        };
        newSceneData.csmLightSpaceTMs = csmPipeline->csmLightSpaceTMs;
        const auto sceneDataDesctiptor =
            renderer.uploadSceneData(newSceneData, csmPipeline->getShadowMap());

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

        const auto colorAttachment = vkinit::
            attachmentInfo(drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
        const auto depthAttachment = vkinit::
            depthAttachmentInfo(depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        const auto renderExtent = VkExtent2D{drawImage.extent.width, drawImage.extent.height};
        const auto renderInfo =
            vkinit::renderingInfo(renderExtent, &colorAttachment, &depthAttachment);

        vkCmdBeginRendering(cmd, &renderInfo);

        meshPipeline->draw(
            cmd, renderExtent, camera, sceneDataDesctiptor, drawCommands, sortedDrawCommands);

        skyboxPipeline->draw(cmd, camera);

        vkCmdEndRendering(cmd);

        vkutil::cmdEndLabel(cmd);
    }

    vkutil::transitionImage(
        cmd,
        drawImage.image,
        VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    // Renderer will render into drawImage after this
}

void GameRenderer::cleanup()
{
    const auto& device = renderer.getDevice();

    vkDeviceWaitIdle(device);

    skyboxPipeline->cleanup(device);
    meshPipeline->cleanup(device);
    csmPipeline->cleanup(device);
    skinningPipeline->cleanup(device);

    renderer.destroyImage(skyboxImage);
    renderer.destroyImage(depthImage);
    renderer.destroyImage(drawImage);

    renderer.cleanup();
}

void GameRenderer::updateDevTools(float dt)
{
    renderer.updateDevTools(dt);
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
