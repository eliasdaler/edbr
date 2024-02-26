#version 460

#extension GL_GOOGLE_include_directive : require

#include "input_structures.glsl"
#include "blinn_phong.glsl"
#include "shadow.glsl"

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;

layout (location = 0) out vec4 outFragColor;

void main()
{
    vec3 baseColor = materialData.baseColor.rgb;
    vec3 diffuse = baseColor * texture(diffuseTex, inUV.xy).rgb;

    vec3 cameraPos = sceneData.cameraPos.xyz;

    vec3 n = normalize(inNormal);
    vec3 l = sceneData.sunlightDirection.xyz;
    vec3 v = normalize(cameraPos - inPos);
    vec3 h = normalize(v + l);
    float NoL = clamp(dot(n, l), 0, 1);

    vec3 fr = blinnPhongBRDF(diffuse, n, v, l, h);

    // shadow
    float occlusion = calculateCSMOcclusion(
            inPos.xyz, cameraPos.xyz, NoL,
            csmShadowMap, sceneData.cascadeFarPlaneZs, sceneData.csmLightSpaceTMs);

    vec3 sunColor = sceneData.sunlightColor.rgb;
    float sunIntensity = sceneData.sunlightColor.w;
    vec3 fragColor = (fr * sunColor) * (sunIntensity * NoL * occlusion);

    // ambient
    vec3 ambientColor = sceneData.ambientColor.rgb;
    float ambientIntensity = sceneData.ambientColor.w;
    fragColor += diffuse * ambientColor.xyz * ambientIntensity;

    // uint cascadeIndex = chooseCascade(inPos, cameraPos, sceneData.cascadeFarPlaneZs);
    // fragColor *= debugShadowsFactor(cascadeIndex);

    // fake gamma correction
    fragColor = pow(fragColor, vec3(1.f/2.2f));

	outFragColor = vec4(fragColor, 1.0f);
}
