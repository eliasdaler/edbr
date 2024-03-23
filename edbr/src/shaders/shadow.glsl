#include "poisson_filtering.glsl"

#define SMOOTH_SHADOW
#define NUM_SHADOW_CASCADES 3

uint chooseCascade(vec3 fragPos, vec3 cameraPos, vec4 cascadeFarPlaneZs) {
    float d = distance(cameraPos.xz, fragPos.xz);
    for (uint i = 0; i < NUM_SHADOW_CASCADES - 1; i += 1) {
        if (d < cascadeFarPlaneZs[i]) {
            return i;
        }
    }
    return NUM_SHADOW_CASCADES - 1;
}

float calculateCSMOcclusion(
        vec3 fragPos, vec3 cameraPos, float NoL,
        uint csmShadowMapId, vec4 cascadeFarPlaneZs,
        mat4 csmLightSpaceTMs[NUM_SHADOW_CASCADES]) {
    uint cascadeIndex = chooseCascade(fragPos, cameraPos, cascadeFarPlaneZs);
    vec4 fragPosLightSpace = csmLightSpaceTMs[cascadeIndex] * vec4(fragPos, 1.0);

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // from [-1;1] to [0;1]
    projCoords.xy = projCoords.xy * vec2(0.5, 0.5) + vec2(0.5);

    float bias = 0.001 * tan(acos(NoL));
    bias = clamp(bias, 0.0, 0.05);
    projCoords.z = projCoords.z + bias;

#ifdef SMOOTH_SHADOW
    float shadow = 0.f;
    int numSamples = 4;
    for (int i = 0; i < numSamples; ++i) {
        vec2 coord = getPoissonDiskCoord(projCoords.xy, i);
        shadow += sampleTextureArrayShadow(csmShadowMapId,
                vec4(coord, cascadeIndex, projCoords.z));
    }
    shadow /= float(numSamples);
    return shadow;
#else
    return texture(csmShadowMap, vec4(projCoords.xy, cascadeIndex, projCoords.z));
#endif
}

vec3 debugShadowsFactor(uint cascadeIndex)  {
    switch (cascadeIndex) {
        case 0: {
            return vec3(1.0f, 0.25f, 0.25f);
        }
        case 1: {
            return vec3(0.25f, 1.0f, 0.25f);
        }
        case 2: {
            return vec3(0.25f, 0.25f, 1.0f);
        }
        default: {
            return vec3(1.0f, 1.0f, 1.0f);
        }
    }
    return vec3(1.0f, 1.0f, 1.0f);
}

