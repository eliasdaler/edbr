#version 460

#extension GL_GOOGLE_include_directive : require

#include "shadow_map_point_pcs.glsl"

layout (location = 0) out vec4 outWorldPos;
layout (location = 1) out vec2 outUV;

void main()
{
    Vertex v = pcs.vertexBuffer.vertices[gl_VertexIndex];

    outWorldPos = pcs.model * vec4(v.position, 1.0f);
    outUV = vec2(v.uv_x, v.uv_y);
    gl_Position = pcs.vpsBuffer.vps[pcs.vpsBufferIndex] * outWorldPos;
}

