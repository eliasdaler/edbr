#version 460

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 0) uniform sampler2D diffuseTex;

void main()
{
    vec3 sunlightDir = vec3(0.371477008, 0.470861048, 0.80018419);
    vec3 ambient = vec3(0.20784314, 0.592156887, 0.56078434);
    float ambientIntensity = 0.05f;

    vec3 diffuseColor = inColor * texture(diffuseTex, inUV).rgb;

    vec3 l = sunlightDir;
    float NoL = clamp(dot(inNormal, l), 0, 1);

    vec3 fragColor = diffuseColor * NoL;

    // ambient
    fragColor += diffuseColor * ambient * ambientIntensity;

    // fake gamma correction
    fragColor = pow(fragColor, vec3(1.f/2.2f));

	outFragColor = vec4(fragColor, 1.0f);
}
