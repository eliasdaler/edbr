#version 460

#extension GL_GOOGLE_include_directive : require

#include "scene_data.glsl"
#include "material_data.glsl"

#include "light.glsl"

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec4 inTangent;
layout (location = 4) in mat3 inTBN;

layout (location = 0) out vec4 outFragColor;

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
        // normal.y = 1 - normal.y; // flip to make OpenGL normal maps work
        normal = inTBN * normalize(normal * 2.0 - 1.0);
        normal = normalize(normal);
    }

#ifdef PBR
    float metallicF = materialData.metallicRoughnessEmissive.r;
    float roughnessF = materialData.metallicRoughnessEmissive.g;

    vec4 metallicRoughness = texture(metallicRoughnessTex, inUV);

    float roughness = roughnessF * metallicRoughness.g;
    // roughness *= roughness; // from perceptual to linear
    roughness = max(roughness, 1e-2); // 0.0 roughness leads to NaNs

    float metallic = metallicF * metallicRoughness.b;

    vec3 dielectricSpecular = vec3(0.04);
    vec3 black = vec3(0.0);
    vec3 diffuseColor = mix(baseColor * (1.0 - dielectricSpecular.r), black, metallic);
    vec3 f0 = mix(dielectricSpecular, baseColor, metallic);
#else
    vec3 diffuseColor = baseColor;
    float metallic = 0.f;
    float roughness = 1.f;
    vec3 f0 = vec3(0.0);
#endif

    vec3 cameraPos = sceneData.cameraPos.xyz;
    vec3 fragPos = inPos.xyz;
    vec3 n = normal;
    vec3 v = normalize(cameraPos - fragPos);

    vec3 fragColor = vec3(0.0);
    for (int i = 0; i < sceneData.numLights; i++) {
        Light light = loadLight(lightsBuffer.lights[i]);
        fragColor += calculateLight(light, fragPos, n, v, cameraPos,
                diffuseColor, roughness, metallic, f0);
    }

    // emissive
    float emissive = materialData.metallicRoughnessEmissive.b;
    vec3 emissiveColor = emissive * texture(emissiveTex, inUV).rgb;
    fragColor += emissiveColor;

    // ambient
    vec3 ambientColor = sceneData.ambientColor.rgb;
    float ambientIntensity = sceneData.ambientColor.w;
    fragColor += baseColor * ambientColor.xyz * ambientIntensity;

#if 0
    // CSM DEBUG
    uint cascadeIndex = chooseCascade(inPos, cameraPos, sceneData.cascadeFarPlaneZs);
    fragColor *= debugShadowsFactor(cascadeIndex);
#endif

#if 0
    // TANGENT DEBUG
    if (inTangent == vec4(0.0)) {
        fragColor = vec3(1.0f, 0.0f, 0.0f);
    }
#endif

#if 1
    // NORMAL DEBUG
	// fragColor = normal;
#endif

	outFragColor = vec4(fragColor, 1.0f);
}
