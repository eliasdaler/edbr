#version 460

#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec4 outColor;

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 color;
    float uv_y;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};

layout (push_constant) uniform constants
{
	mat4 mvp;
    vec4 color;
	VertexBuffer vertexBuffer;
} pushConstants;

void main()
{
    Vertex v = pushConstants.vertexBuffer.vertices[gl_VertexIndex];
    gl_Position = pushConstants.mvp * vec4(v.position, 1.0);
    outUV = vec2(v.uv_x, v.uv_y);
    outColor = pushConstants.color * vec4(v.color, 1.0);
}

