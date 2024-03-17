#extension GL_EXT_buffer_reference : require

#include "vertex.glsl"

layout (buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout (push_constant) uniform constants
{
    mat4 transform;
    VertexBuffer vertexBuffer;
    uint materialID;
} pushConstants;

