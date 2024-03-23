#ifndef VERTEX_GLSL
#define VERTEX_GLSL

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 tangent;
};

#endif // VERTEX_GLSL
