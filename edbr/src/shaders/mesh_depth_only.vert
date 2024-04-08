#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "mesh_pcs.glsl"

layout (location = 0) out vec2 outUV;

void main()
{
    Vertex v = pcs.vertexBuffer.vertices[gl_VertexIndex];

    outUV = vec2(v.uv_x, v.uv_y);
    gl_Position = pcs.vp * pcs.transform * vec4(v.position, 1.0f);
}
