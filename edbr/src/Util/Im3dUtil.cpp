#include <edbr/Util/Im3dUtil.h>

#include <edbr/Graphics/FrustumCulling.h>

void Im3dDrawFrustum(const Camera& camera)
{
    const auto corners = edge::calculateFrustumCornersWorldSpace(camera);

    // near plane
    Im3d::PushColor(Im3d::Color_Magenta);
    Im3d::DrawQuad(
        glm2im3d(corners[0]), glm2im3d(corners[1]), glm2im3d(corners[2]), glm2im3d(corners[3]));

    // far plane
    Im3d::DrawQuad(
        glm2im3d(corners[4]), glm2im3d(corners[5]), glm2im3d(corners[6]), glm2im3d(corners[7]));
    Im3d::PopColor();

    // left plane
    Im3d::PushColor(Im3d::Color_Orange);
    Im3d::DrawQuad(
        glm2im3d(corners[4]), glm2im3d(corners[5]), glm2im3d(corners[1]), glm2im3d(corners[0]));

    // right plane
    Im3d::DrawQuad(
        glm2im3d(corners[7]), glm2im3d(corners[6]), glm2im3d(corners[2]), glm2im3d(corners[3]));
    Im3d::PopColor();

    /* for (std::size_t i = 0; i < corners.size(); ++i) {
        Im3dText(corners[i], 8.f, RGBColor{255, 255, 255, 255}, std::to_string(i).c_str());
    } */

    bool drawNormals = false;
    if (drawNormals) {
        const auto normalLength = 0.25f;
        const auto normalColor = glm::vec4{1.f, 0.0f, 1.0f, 1.f};
        const auto normalColorEnd = glm::vec4{0.f, 1.f, 1.f, 1.f};
        const auto frustum = edge::createFrustumFromCamera(camera);

        const auto npc = (corners[0] + corners[1] + corners[2] + corners[3]) / 4.f;
        Im3d::DrawLine(
            glm2im3d(npc),
            glm2im3d(npc + frustum.nearFace.normal * normalLength),
            1.f,
            Im3d::Color_Cyan);

        const auto fpc = (corners[4] + corners[5] + corners[6] + corners[7]) / 4.f;
        Im3d::DrawLine(
            glm2im3d(fpc),
            glm2im3d(fpc + frustum.farFace.normal * normalLength),
            1.f,
            Im3d::Color_Cyan);

        const auto lpc = (corners[4] + corners[5] + corners[1] + corners[0]) / 4.f;
        Im3d::DrawLine(
            glm2im3d(lpc),
            glm2im3d(lpc + frustum.leftFace.normal * normalLength),
            1.f,
            Im3d::Color_Cyan);

        const auto rpc = (corners[7] + corners[6] + corners[2] + corners[3]) / 4.f;
        Im3d::DrawLine(
            glm2im3d(rpc),
            glm2im3d(rpc + frustum.rightFace.normal * normalLength),
            1.f,
            Im3d::Color_Cyan);

        const auto bpc = (corners[0] + corners[4] + corners[7] + corners[3]) / 4.f;
        Im3d::DrawLine(
            glm2im3d(bpc),
            glm2im3d(bpc + frustum.bottomFace.normal * normalLength),
            1.f,
            Im3d::Color_Cyan);

        const auto tpc = (corners[1] + corners[5] + corners[6] + corners[2]) / 4.f;
        Im3d::DrawLine(
            glm2im3d(tpc),
            glm2im3d(tpc + frustum.topFace.normal * normalLength),
            1.f,
            Im3d::Color_Cyan);
    }
}
