#version 460

#extension GL_GOOGLE_include_directive : require

#include "input_structures.glsl"
#include "blinn_phong.glsl"

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;

layout (location = 0) out vec4 outFragColor;

uint chooseCascade(vec3 fragPos) {
    vec2 fp = sceneData.cameraPos.xz;
    float d = distance(fp, fragPos.xz);
    for (uint i = 0; i < NUM_SHADOW_CASCADES - 1; i += 1) {
        if (d < sceneData.cascadeFarPlaneZs[i]) {
            return i;
        }
    }
    return 2;
}

void main()
{
    vec3 baseColor = materialData.baseColor.rgb;
    vec3 diffuse = baseColor * texture(diffuseTex, inUV.xy).rgb;

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

    // shadow
    uint cascadeIndex = chooseCascade(inPos);
    vec4 fragPosLightSpace = sceneData.csmLightSpaceTMs[cascadeIndex] * vec4(inPos, 1.0);

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    float bias = 0.001 * tan(acos(NoL));
    bias = clamp(bias, 0.0, 0.1);
    projCoords.z = projCoords.z + bias;

    // from [-1;1] to [0;1]
    vec2 coord = projCoords.xy * vec2(0.5, 0.5) + vec2(0.5);
    float fragDepth = projCoords.z;
    float occlusion = texture(csmShadowMap, vec4(coord, cascadeIndex, fragDepth));

    vec3 fragColor = (fr * sunColor) * (sunIntensity * NoL * occlusion);

    // ambient
    fragColor += diffuse * ambientColor.xyz * ambientIntensity;

    // fake gamma correction
    fragColor = pow(fragColor, vec3(1.f/2.2f));

	outFragColor = vec4(fragColor, 1.0f);
}
