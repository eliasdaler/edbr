#version 460

#extension GL_GOOGLE_include_directive : require

#include "vertex.glsl"
#include "materials.glsl"

layout (location = 0) out vec2 outUV;

layout (push_constant) uniform constants
{
	mat4 mvp;
	VertexBuffer vertexBuffer;
    MaterialsBuffer materials;
    uint materialID;
} pcs;

void main()
{
    Vertex v = pcs.vertexBuffer.vertices[gl_VertexIndex];

    outUV = vec2(v.uv_x, v.uv_y);
    gl_Position = pcs.mvp * vec4(v.position, 1.0f);
}
