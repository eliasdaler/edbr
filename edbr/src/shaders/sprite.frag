#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "bindless.glsl"
#include "sprite_pcs.glsl"

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inColor;
layout (location = 2) flat in uint textureID;

layout (location = 0) out vec4 outColor;

#define SPRITE_SHADER_ID 0
#define TEXT_SHADER_ID   1

void main()
{
    vec4 texColor = sampleTexture2DNearest(textureID, inUV);
    if (pcs.shaderID == TEXT_SHADER_ID) {
        texColor = vec4(1.0, 1.0, 1.0, texColor.r);
    }

    if (texColor.a < 0.1) {
        discard;
    }
    outColor = inColor * texColor;
}
