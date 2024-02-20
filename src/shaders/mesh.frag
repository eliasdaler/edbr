#version 460

#extension GL_GOOGLE_include_directive : require

#include "input_structures.glsl"
#include "blinn_phong.glsl"

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;

layout (location = 0) out vec4 outFragColor;

void main()
{
    vec3 diffuse = texture(diffuseTex, inUV.xy).rgb;

    vec3 cameraPos = sceneData.cameraPos.xyz;

    vec3 sunDir = sceneData.sunlightDirection.xyz;
    vec3 sunColor = sceneData.sunlightColor.rgb;
    float sunIntensity = sceneData.sunlightColor.w;

    vec3 ambientColor = sceneData.ambientColor.rgb;
    float ambientIntensity = sceneData.ambientColor.w;

    vec3 n = normalize(inNormal);
    vec3 l = sceneData.sunlightDirection.xyz;
    vec3 v = normalize(cameraPos - inPos);
    vec3 h = normalize(v + l);
    float NoL = clamp(dot(n, l), 0, 1);

    vec3 fr = blinnPhongBRDF(diffuse, n, v, l, h);

    vec3 fragColor = (fr * sunColor) * (sunIntensity * NoL);

    // ambient
    fragColor += diffuse * ambientColor.xyz * ambientIntensity;

    // fake gamma correction
    fragColor = pow(fragColor, vec3(1.f/2.2f));

	outFragColor = vec4(fragColor, 1.0f);
}
