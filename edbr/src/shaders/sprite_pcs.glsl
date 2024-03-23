#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout: require

struct SpriteDrawCommand {
    mat4 transform;
    vec2 uv0;
    vec2 uv1;
    vec4 color;
    uint textureID;
    uint shaderID;
    vec2 padding;
};

layout (buffer_reference, scalar) readonly buffer SpriteDrawBuffer {
	SpriteDrawCommand commands[];
};

layout (push_constant) uniform constants
{
    mat4 viewProj;
    SpriteDrawBuffer drawBuffer;
    uint shaderID;
} pcs;

