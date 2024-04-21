#include <edbr/Util/Im3dUtil.h>

#include <edbr/Graphics/FrustumCulling.h>

void Im3dDrawFrustum(const Camera& camera)
{
    const auto corners = edge::calculateFrustumCornersWorldSpace(camera);

    // near plane
    Im3d::PushColor(Im3d::Color_Magenta);
    Im3d::DrawQuad(
        glm2im3d(glm::vec3{corners[0]}),
        glm2im3d(glm::vec3{corners[1]}),
        glm2im3d(glm::vec3{corners[2]}),
        glm2im3d(glm::vec3{corners[3]}));

    // far plane
    Im3d::DrawQuad(
        glm2im3d(glm::vec3{corners[4]}),
        glm2im3d(glm::vec3{corners[5]}),
        glm2im3d(glm::vec3{corners[6]}),
        glm2im3d(glm::vec3{corners[7]}));
    Im3d::PopColor();

    // left plane
    Im3d::PushColor(Im3d::Color_Orange);
    Im3d::DrawQuad(
        glm2im3d(glm::vec3{corners[4]}),
        glm2im3d(glm::vec3{corners[5]}),
        glm2im3d(glm::vec3{corners[1]}),
        glm2im3d(glm::vec3{corners[0]}));

    // right plane
    Im3d::DrawQuad(
        glm2im3d(glm::vec3{corners[7]}),
        glm2im3d(glm::vec3{corners[6]}),
        glm2im3d(glm::vec3{corners[2]}),
        glm2im3d(glm::vec3{corners[3]}));
    Im3d::PopColor();

    /* for (std::size_t i = 0; i < corners.size(); ++i) {
        Im3dText(corners[i], 8.f, RGBColor{255, 255, 255, 255}, std::to_string(i).c_str());
    } */
}
