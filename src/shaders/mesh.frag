#version 460

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 ivUV;
layout (location = 2) in vec3 inNormal;

layout (location = 0) out vec4 outFragColor;

void main()
{
    vec3 l = vec3(0.8, 0.1, 0.1);
    float NdotL = dot(inNormal, l);

	outFragColor = vec4(inColor * NdotL, 1.0f);
}
