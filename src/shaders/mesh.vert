#version 460
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;

struct Vertex {
	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{
	Vertex vertices[];
};

layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	VertexBuffer vertexBuffer;
} PushConstants;

void main()
{
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    vec3 pos = v.position;
    pos.y = -pos.y + 0.75;

    gl_Position = PushConstants.render_matrix * vec4(pos, 1.0f);
	outColor = v.color.xyz;
    outUV = vec2(v.uv_x, v.uv_y);
    outNormal = v.normal;
}
