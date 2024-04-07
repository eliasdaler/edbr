#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Body/BodyManager.h>
#include <Jolt/Renderer/DebugRenderer.h>

#include <edbr/Graphics/FrustumCulling.h>

class Camera;

class JoltDebugRenderer : public JPH::DebugRenderer {
public:
    struct BatchImpl : public JPH::RefTargetVirtual {
        std::vector<Vertex> vtx;
        std::vector<JPH::uint32> idx;
        int mRefCount = 0;

        BatchImpl(const Vertex* inVert, int inVertCount, const JPH::uint32* inIdx, int inIdxCount)
        {
            vtx.resize(inVertCount);
            for (int i = 0; i < inVertCount; ++i) {
                vtx[i] = inVert[i];
            }

            if (inIdx) {
                idx.resize(inIdxCount);
                for (int i = 0; i < inIdxCount; ++i) {
                    idx[i] = inIdx[i];
                }
            } else {
                idx.resize(inIdxCount);
                for (int i = 0; i < inIdxCount; ++i)
                    idx[i] = i;
            }
        }
        void AddRef() override { mRefCount++; }
        void Release() override
        {
            if (--mRefCount == 0) delete this;
        }
    };

    JoltDebugRenderer();

    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
    void DrawTriangle(
        JPH::RVec3Arg inV1,
        JPH::RVec3Arg inV2,
        JPH::RVec3Arg inV3,
        JPH::ColorArg inColor,
        ECastShadow inCastShadow = ECastShadow::Off) override;
    Batch CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) override;
    Batch CreateTriangleBatch(
        const Vertex* inVertices,
        int inVertexCount,
        const std::uint32_t* inIndices,
        int inIndexCount) override;
    void DrawGeometry(
        JPH::RMat44Arg inModelMatrix,
        const JPH::AABox& inWorldSpaceBounds,
        float inLODScaleSq,
        JPH::ColorArg inModelColor,
        const GeometryRef& inGeometry,
        ECullMode inCullMode,
        ECastShadow inCastShadow,
        EDrawMode inDrawMode) override;
    void DrawText3D(
        JPH::RVec3Arg inPosition,
        const std::string_view& inString,
        JPH::ColorArg inColor,
        float inHeight) override;

    void setCamera(const Camera& camera);

    JPH::BodyManager::DrawSettings bodyDrawSettings;

    Frustum frustum;
    JPH::Vec3 cameraPos;
    float maxRenderDistSq{64.f * 64.f};
    bool drawLinesDepthTest{true};
    float solidShapeAlpha{0.3f};
};
