#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "scene_data.glsl"
#include "vertex.glsl"

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;

layout (buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};

layout (push_constant) uniform constants
{
	mat4 transform;
	VertexBuffer vertexBuffer;
} pushConstants;

void main()
{
    Vertex v = pushConstants.vertexBuffer.vertices[gl_VertexIndex];

    vec4 worldPos = pushConstants.transform * vec4(v.position, 1.0f);

    gl_Position = sceneData.viewProj * worldPos;
	outPos = worldPos.xyz;
    outUV = vec2(v.uv_x, v.uv_y);
    outNormal = v.normal;
}
