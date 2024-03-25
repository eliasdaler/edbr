#version 460

#extension GL_GOOGLE_include_directive : require

#include "im3d_pcs.glsl"

layout(location=0) out VertexData vData;

void main()
{
    const VertexFormat v = pcs.vertices.vertices[gl_VertexIndex];
    const vec4 color = unpackUnorm4x8(v.color);
    vData.m_color = color.abgr; // swizzle to correct endianness
    vData.m_color.a *= smoothstep(0.0, 1.0, v.positionSize.w / kAntialiasing);
    vData.m_size = max(v.positionSize.w, kAntialiasing);
    gl_Position = pcs.uViewProjMatrix * vec4(v.positionSize.xyz, 1.0);
    gl_PointSize = vData.m_size;
}
