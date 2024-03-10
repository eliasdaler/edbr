#version 460

layout (set = 0, binding = 0) uniform sampler2D tex;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inColor;

layout (location = 0) out vec4 outColor;

void main()
{
    vec4 texColor = inColor * texture(tex, inUV);
    if (texColor.a < 0.1) {
        discard;
    }
    outColor = texColor;
}
