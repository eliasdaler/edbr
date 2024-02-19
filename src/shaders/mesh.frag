#version 460

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 ivUV;

layout (location = 0) out vec4 outFragColor;

void main()
{
	outFragColor = vec4(inColor, 1.0f);
}
