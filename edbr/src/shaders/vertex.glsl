#ifndef VERTEX_GLSL
#define VERTEX_GLSL

#extension GL_EXT_buffer_reference : require

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 tangent;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};

#endif // VERTEX_GLSL
