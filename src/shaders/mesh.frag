#version 460

#extension GL_GOOGLE_include_directive : require

#include "input_structures.glsl"
#include "shadow.glsl"

#include "pbr.glsl"
#include "blinn_phong.glsl"

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;

layout (location = 0) out vec4 outFragColor;

#define PBR

void main()
{
    vec4 albedo = texture(diffuseTex, inUV.xy);
    vec3 baseColor = materialData.baseColor.rgb * albedo.rgb;

#ifdef PBR
    float metallic = materialData.metallicRoughness.r;
    float roughness = materialData.metallicRoughness.g;
    roughness = max(roughness, 1e-6); // 0.0 roughness leads to NaNs

    // TODO: figure out if we need to do this!
    // roughness *= roughness; // from perceptual to linear
    vec3 dielectricSpecular = vec3(0.04);
    vec3 black = vec3(0.0);
    vec3 diffuseColor = mix(baseColor * (1.0 - dielectricSpecular.r), black, metallic);
    vec3 f0 = mix(dielectricSpecular, baseColor, metallic);
#else
    vec3 diffuseColor = baseColor;
#endif

    vec3 cameraPos = sceneData.cameraPos.xyz;

    vec3 n = normalize(inNormal);
    vec3 l = sceneData.sunlightDirection.xyz;
    vec3 v = normalize(cameraPos - inPos);
    vec3 h = normalize(v + l);
    float NoL = clamp(dot(n, l), 0, 1);

    vec3 sunColor = sceneData.sunlightColor.rgb;
    float sunIntensity = sceneData.sunlightColor.w;

#ifdef PBR
    vec3 fr = pbrBRDF(
            diffuseColor, roughness, metallic, f0,
            n, v, l, h);
    // TODO: figure out how to properly compute light intensity for PBR
    sunIntensity *= 4.6;
#else
    vec3 fr = blinnPhongBRDF(diffuseColor, n, v, l, h);
#endif

    // shadow
    float occlusion = calculateCSMOcclusion(
            inPos.xyz, cameraPos.xyz, NoL,
            csmShadowMap, sceneData.cascadeFarPlaneZs, sceneData.csmLightSpaceTMs);

    vec3 fragColor = (fr * sunColor) * (sunIntensity * NoL * occlusion);

    // ambient
    vec3 ambientColor = sceneData.ambientColor.rgb;
    float ambientIntensity = sceneData.ambientColor.w;
    fragColor += baseColor * ambientColor.xyz * ambientIntensity;

    // uint cascadeIndex = chooseCascade(inPos, cameraPos, sceneData.cascadeFarPlaneZs);
    // fragColor *= debugShadowsFactor(cascadeIndex);

	outFragColor = vec4(fragColor, 1.0f);
}
