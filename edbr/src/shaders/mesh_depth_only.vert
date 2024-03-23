#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "vertex.glsl"

layout (buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};

layout (push_constant) uniform constants
{
	mat4 mvp;
	VertexBuffer vertexBuffer;
} pcs;

void main()
{
    Vertex v = pcs.vertexBuffer.vertices[gl_VertexIndex];

    gl_Position = pcs.mvp * vec4(v.position, 1.0f);
}
