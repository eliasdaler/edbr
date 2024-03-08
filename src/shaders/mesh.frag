#version 460

#extension GL_GOOGLE_include_directive : require

#include "scene_data.glsl"
#include "material_data.glsl"

#include "shadow.glsl"

#include "pbr.glsl"
#include "blinn_phong.glsl"

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec4 inTangent;
layout (location = 4) in mat3 inTBN;

layout (location = 0) out vec4 outFragColor;

#define PBR

void main()
{
    vec4 albedo = texture(diffuseTex, inUV);
    vec3 baseColor = materialData.baseColor.rgb * albedo.rgb;

    vec3 normal = normalize(inNormal).rgb;
    if (inTangent != vec4(0.0)) {
        // FIXME: sometimes Blender doesn't export tangents for some objects
        // for some reason. When we will start computing tangents manually,
        // this check can be removed
        normal = texture(normalMapTex, inUV).rgb;
        normal = inTBN * normalize(normal * 2.0 - 1.0);
    }

#ifdef PBR
    float metallicF = materialData.metallicRoughnessEmissive.r;
    float roughnessF = materialData.metallicRoughnessEmissive.g;

    vec4 metallicRoughness = texture(metallicRoughnessTex, inUV);

    float roughness = roughnessF * metallicRoughness.g;
    roughness *= roughness; // from perceptual to linear
    roughness = max(roughness, 1e-6); // 0.0 roughness leads to NaNs

    float metallic = metallicF * metallicRoughness.b;

    vec3 dielectricSpecular = vec3(0.04);
    vec3 black = vec3(0.0);
    vec3 diffuseColor = mix(baseColor * (1.0 - dielectricSpecular.r), black, metallic);
    vec3 f0 = mix(dielectricSpecular, baseColor, metallic);
#else
    vec3 diffuseColor = baseColor;
#endif

    vec3 cameraPos = sceneData.cameraPos.xyz;

    // vec3 n = normalize(normal);
    vec3 n = normal;
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
    sunIntensity *= 2.6;
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

    // emissive
    float emissive = materialData.metallicRoughnessEmissive.b;
    vec3 emissiveColor = emissive * texture(emissiveTex, inUV).rgb;
    fragColor += emissiveColor;

    // CSM debug
#if 0
    uint cascadeIndex = chooseCascade(inPos, cameraPos, sceneData.cascadeFarPlaneZs);
    fragColor *= debugShadowsFactor(cascadeIndex);
#endif

    // tangent debug
#if 0
    if (inTangent == vec4(0.0)) {
        fragColor = vec3(1.0f, 0.0f, 0.0f);
    }
#endif

	outFragColor = vec4(fragColor, 1.0f);
}
