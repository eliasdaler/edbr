#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <im3d.h>

#include <edbr/Graphics/Color.h>
#include <edbr/Math/Rect.h>

inline Im3d::Color edbr2im3d(const RGBColor& c)
{
    return {
        (float)c.r / 255.f,
        (float)c.g / 255.f,
        (float)c.b / 255.f,
        (float)c.a / 255.f,
    };
}

inline Im3d::Vec2 glm2im3d(const glm::vec2& v)
{
    return {v.x, v.y};
}

inline Im3d::Vec3 glm2im3d(const glm::vec3& v)
{
    return {v.x, v.y, v.z};
}

inline glm::vec2 im3d2glm(const Im3d::Vec2& v)
{
    return {v.x, v.y};
}

inline glm::vec3 im3d2glm(const Im3d::Vec3& v)
{
    return {v.x, v.y, v.z};
}

inline Im3d::Mat4 glm2im3d(glm::mat4 m)
{
    // clang-format off
    return {
        m[0][0], m[1][0], m[2][0], m[3][0],
        m[0][1], m[1][1], m[2][1], m[3][1],
        m[0][2], m[1][2], m[2][2], m[3][2],
        m[0][3], m[1][3], m[2][3], m[3][3]
    };
    // clang-format on
}

inline glm::mat4 im3d2glm(const Im3d::Mat4& m)
{
    // clang-format off
    return glm::mat4{
        m(0, 0), m(1, 0), m(2, 0), m(3, 0),
        m(0, 1), m(1, 1), m(2, 1), m(3, 1),
        m(0, 2), m(1, 2), m(2, 2), m(3, 2),
        m(0, 3), m(1, 3), m(2, 3), m(3, 3)
    };
    // clang-format on
}

inline void Im3dDrawArrow(
    const RGBColor& color,
    glm::vec3 start,
    glm::vec3 end,
    float arrowSize = 5.0)
{
    Im3d::PushDrawState();
    Im3d::SetSize(arrowSize);
    Im3d::SetColor(edbr2im3d(color));
    Im3d::DrawArrow(glm2im3d(start), glm2im3d(end));
    Im3d::PopDrawState();
}

inline void Im3dText(
    const glm::vec3& position,
    float textSize,
    const RGBColor& color,
    const char* text)
{
    static Im3d::TextFlags textFlags = Im3d::TextFlags_Default;
    Im3d::Text(glm2im3d(position), textSize, edbr2im3d(color), textFlags, text);
}

inline void Im3dRect(const math::FloatRect& rect, const RGBColor& borderColor)
{
    const auto a = glm2im3d(glm::vec3{rect.getTopLeftCorner(), 0.f});
    const auto b = glm2im3d(glm::vec3{rect.getTopRightCorner(), 0.f});
    const auto c = glm2im3d(glm::vec3{rect.getBottomRightCorner(), 0.f});
    const auto d = glm2im3d(glm::vec3{rect.getBottomLeftCorner(), 0.f});

    Im3d::PushDrawState();
    Im3d::SetColor(edbr2im3d(borderColor));
    Im3d::DrawQuad(a, b, c, d);
    Im3d::PopDrawState();
}

inline void Im3dRectFilled(const math::FloatRect& rect, const RGBColor& fillColor)
{
    // assert(false && "doesn't work for some reason");
    const auto a = glm2im3d(glm::vec3{rect.getTopLeftCorner(), 0.f});
    const auto b = glm2im3d(glm::vec3{rect.getTopRightCorner(), 0.f});
    const auto c = glm2im3d(glm::vec3{rect.getBottomRightCorner(), 0.f});
    const auto d = glm2im3d(glm::vec3{rect.getBottomLeftCorner(), 0.f});

    Im3d::PushDrawState();
    Im3d::SetColor(edbr2im3d(fillColor));
    // CCW order so that it's not culled
    Im3d::DrawQuadFilled(a, d, c, b);
    Im3d::PopDrawState();
}
