#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "sprite_pcs.glsl"

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec4 outColor;
layout (location = 2) flat out uint textureID;

void main()
{
    uint b = 1 << (gl_VertexIndex % 6);
    vec2 baseCoord = vec2((0x1C & b) != 0, (0xE & b) != 0);

    SpriteDrawCommand command = pushConstants.drawBuffer.commands[gl_InstanceIndex];

    gl_Position = pushConstants.viewProj * command.transform * vec4(baseCoord, 0.f, 1.f);
    outUV = (1.f - baseCoord) * command.uv0 + baseCoord * command.uv1;
    outColor = command.color;
    textureID = command.textureID;
}

