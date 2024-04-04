#include "JoltDebugRenderer.h"

#include <im3d.h>

#include <edbr/DevTools/Im3dState.h>
#include <edbr/Math/AABB.h>
#include <edbr/Util/JoltUtil.h>

JoltDebugRenderer::JoltDebugRenderer()
{
    bodyDrawSettings.mDrawShapeWireframe = true;
    bodyDrawSettings.mDrawWorldTransform = true;

    bodyDrawSettings = JPH::BodyManager::DrawSettings{
        .mDrawGetSupportFunction = false, ///< Draw the GetSupport() function, used for convex
                                          ///< collision detection
        .mDrawSupportDirection = false, ///< When drawing the support function, also draw which
                                        ///< direction mapped to a specific support point
        .mDrawGetSupportingFace = false, ///< Draw the faces that were found colliding during
                                         ///< collision detection
        .mDrawShape = false, ///< Draw the shapes of all bodies
        .mDrawShapeWireframe = true, ///< When mDrawShape is true and this is true, the shapes
                                     ///< will be drawn in wireframe instead of solid.
        .mDrawShapeColor = JPH::BodyManager::EShapeColor::InstanceColor, ///< Coloring scheme to use
                                                                         ///< for shapes
        .mDrawBoundingBox = false, ///< Draw a bounding box per body
        .mDrawCenterOfMassTransform = false, ///< Draw the center of mass for each body
        .mDrawWorldTransform = false, ///< Draw the world transform (which can be different than the
                                      ///< center of mass) for each body
        .mDrawVelocity = false, ///< Draw the velocity vector for each body
        .mDrawMassAndInertia = false, ///< Draw the mass and inertia (as the box equivalent) for
                                      ///< each body
        .mDrawSleepStats = false, ///< Draw stats regarding the sleeping algorithm of each body
    };

    DebugRenderer::Initialize();
}

void JoltDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
    Im3d::PushLayerId(
        drawLinesDepthTest ? Im3dState::WorldWithDepthLayer : Im3dState::WorldNoDepthLayer);

    Im3d::
        DrawLine(util::joltToIm3d(inFrom), util::joltToIm3d(inTo), 3.f, util::joltToIm3d(inColor));

    Im3d::PopLayerId();
}

void JoltDebugRenderer::DrawTriangle(
    JPH::RVec3Arg inV1,
    JPH::RVec3Arg inV2,
    JPH::RVec3Arg inV3,
    JPH::ColorArg inColor,
    ECastShadow inCastShadow)
{}

JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(
    const Triangle* inTriangles,
    int inTriangleCount)
{
    return new BatchImpl(inTriangles->mV, inTriangleCount * 3, nullptr, inTriangleCount * 3);
}

JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(
    const Vertex* inVertices,
    int inVertexCount,
    const std::uint32_t* inIndices,
    int inIndexCount)
{
    return new BatchImpl(inVertices, inVertexCount, inIndices, inIndexCount);
}

namespace
{
glm::vec3 joltToGLM(const JPH::Vec3& v)
{
    return glm::vec3{v.GetX(), v.GetY(), v.GetZ()};
}
}

void JoltDebugRenderer::DrawGeometry(
    JPH::RMat44Arg inModelMatrix,
    const JPH::AABox& inWorldSpaceBounds,
    float inLODScaleSq,
    JPH::ColorArg inModelColor,
    const GeometryRef& inGeometry,
    ECullMode inCullMode,
    ECastShadow inCastShadow,
    EDrawMode inDrawMode)
{
    if (inWorldSpaceBounds.GetSqDistanceTo(cameraPos) > maxRenderDistSq) {
        return;
    }

    const auto aabb = math::AABB{
        .min = joltToGLM(inWorldSpaceBounds.mMin),
        .max = joltToGLM(inWorldSpaceBounds.mMax),
    };
    bool inFrustum = edge::isInFrustum(frustum, aabb);

    /*
    Im3d::PushLayerId(Im3dState::WorldWithDepthLayer);
    Im3d::PushColor(inFrustum ? Im3d::Color_Green : Im3d::Color_Red);
    Im3d::DrawAlignedBox(joltToIm3d(inWorldSpaceBounds.mMin), joltToIm3d(inWorldSpaceBounds.mMax));
    Im3d::PopColor();
    Im3d::PopLayerId();
    */

    if (!inFrustum) {
        return;
    }

    float dist_sq = inWorldSpaceBounds.GetSqDistanceTo(cameraPos);
    int lodNum = 0;
    for (size_t lod = 0; lod < inGeometry->mLODs.size(); ++lod) {
        if (dist_sq <= inLODScaleSq * JPH::Square(inGeometry->mLODs[lod].mDistance)) {
            lodNum = lod;
            break;
        }
    }

    const auto layer =
        drawLinesDepthTest ? Im3dState::WorldWithDepthLayer : Im3dState::WorldNoDepthLayer;
    /* if (lodNum != 0 && drawLinesDepthTest) {
        layer = Im3dState::WorldNoDepthLayer;
    } */

    Im3d::PushLayerId(layer);

    const auto& b =
        *static_cast<const BatchImpl*>(inGeometry->mLODs[lodNum].mTriangleBatch.GetPtr());
    for (std::size_t fi = 0; fi < b.idx.size(); fi += 3) {
        JPH::Float3 v0, v1, v2;
        (inModelMatrix * JPH::Vec3(b.vtx[b.idx[fi + 0]].mPosition)).StoreFloat3(&v0);
        (inModelMatrix * JPH::Vec3(b.vtx[b.idx[fi + 1]].mPosition)).StoreFloat3(&v1);
        (inModelMatrix * JPH::Vec3(b.vtx[b.idx[fi + 2]].mPosition)).StoreFloat3(&v2);

        using namespace util;
        float lineWidth = lodNum != 0 ? 1.f : 3.f;
        Im3d::DrawLine(joltToIm3d(v0), joltToIm3d(v1), lineWidth, joltToIm3d(inModelColor));
        Im3d::DrawLine(joltToIm3d(v1), joltToIm3d(v2), lineWidth, joltToIm3d(inModelColor));
        Im3d::DrawLine(joltToIm3d(v2), joltToIm3d(v0), lineWidth, joltToIm3d(inModelColor));
    }

    Im3d::PopLayerId();
}

void JoltDebugRenderer::DrawText3D(
    JPH::RVec3Arg inPosition,
    const std::string_view& inString,
    JPH::ColorArg inColor,
    float inHeight)
{}

void JoltDebugRenderer::setCamera(const Camera& camera)
{
    frustum = edge::createFrustumFromCamera(camera);
    cameraPos = util::glmToJolt(camera.getPosition());
}

void JoltDebugRenderer::setDrawLinesDepthTest(bool b)
{
    drawLinesDepthTest = b;
}

void JoltDebugRenderer::setDrawShapeWireFrame(bool b)
{
    bodyDrawSettings.mDrawShape = b;
}

void JoltDebugRenderer::setDrawBoundingBox(bool b)
{
    bodyDrawSettings.mDrawBoundingBox = b;
}
