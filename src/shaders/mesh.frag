#version 460

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 0) uniform SceneData{
	mat4 view;
	mat4 proj;
	mat4 viewProj;
	vec4 ambientColor; // w - ambient intensity
	vec4 sunlightDirection;
	vec4 sunlightColor; // w - sun intensity
} sceneData;

layout (set = 1, binding = 0) uniform sampler2D diffuseTex;

void main()
{
    vec3 diffuseColor = inColor * texture(diffuseTex, inUV).rgb;

    vec3 l = sceneData.sunlightDirection.xyz;
    float NoL = clamp(dot(inNormal, l), 0, 1);

    vec3 fragColor = diffuseColor * NoL;

    // ambient
    fragColor += diffuseColor * sceneData.ambientColor.xyz * sceneData.ambientColor.w;

    // fake gamma correction
    fragColor = pow(fragColor, vec3(1.f/2.2f));

	outFragColor = vec4(fragColor, 1.0f);
}
