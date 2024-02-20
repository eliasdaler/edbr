#version 460
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;

struct Vertex {
	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer{
	Vertex vertices[];
};

layout (set = 0, binding = 0) uniform SceneData{
	mat4 view;
	mat4 proj;
	mat4 viewProj;
    vec4 cameraPos;
	vec4 ambientColor; // w - ambient intensity
	vec4 sunlightDirection;
	vec4 sunlightColor; // w - sun intensity
} sceneData;

layout (push_constant) uniform constants
{
	mat4 transform;
	VertexBuffer vertexBuffer;
} PushConstants;

void main()
{
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    vec4 worldPos = PushConstants.transform * vec4(v.position, 1.0f);
    gl_Position = sceneData.viewProj * worldPos;

	outPos = worldPos.xyz;
    outUV = vec2(v.uv_x, v.uv_y);
    outNormal = v.normal;
}
