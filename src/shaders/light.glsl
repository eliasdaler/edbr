#ifndef LIGHT_H
#define LIGHT_H

#include "light_data.glsl"

#include "pbr.glsl"
#include "blinn_phong.glsl"

#include "shadow.glsl"

#define PBR

#define TYPE_DIRECTIONAL_LIGHT 0
#define TYPE_POINT_LIGHT 1
#define TYPE_SPOT_LIGHT 2

// layout in the shader
struct Light
{
    int type;
    vec3 position;

    vec3 direction;
    float range;

    vec3 color;
    float intensity;

    vec2 scaleOffset; // spot light only
    int shadowMapIndex;
};

Light loadLight(LightData data) {
    Light light;

    light.position = data.positionAndType.xyz;
    light.type = int(data.positionAndType.w);

    light.direction = data.directionAndRange.xyz;
    light.range = data.directionAndRange.w;

    light.color = data.colorAndIntensity.rgb;
    light.intensity = data.colorAndIntensity.w;

    light.scaleOffset = data.scaleOffsetAndSMIndexAndUnused.xy;
    light.shadowMapIndex = int(data.scaleOffsetAndSMIndexAndUnused.z);

    return light;
}

float calculateDistanceAttenuation(float dist, float range)
{
    float d = clamp(1.0 - pow((dist/range), 4.0), 0.0, 1.0);
    return d / (dist*dist);
}

// See KHR_lights_punctual spec - formulas are taken from it
float calculateAngularAttenuation(
        vec3 lightDir, vec3 l,
        vec2 scaleOffset) {
    float cd = dot(lightDir, l);
    float angularAttenuation = clamp(cd * scaleOffset.x + scaleOffset.y, 0.0, 1.0);
    angularAttenuation *= angularAttenuation;
    // angularAttenuation = smoothstep(0, 1, angularAttenuation);
    return angularAttenuation;
}

float calculateAttenuation(vec3 pos, vec3 l, Light light) {
    float dist = length(light.position - pos);
    float atten = calculateDistanceAttenuation(dist, light.range);
    if (light.type == TYPE_SPOT_LIGHT) {
        atten *= calculateAngularAttenuation(light.direction, l, light.scaleOffset);
    }
    return atten;
}

float calculateOcclusion(Light light, vec3 fragPos, vec3 cameraPos, float NoL)
{
    float occlusion = 1.0;
    if (light.type == TYPE_DIRECTIONAL_LIGHT) {
        occlusion = calculateCSMOcclusion(
                fragPos, cameraPos, NoL,
                csmShadowMap, sceneData.cascadeFarPlaneZs, sceneData.csmLightSpaceTMs);
    }
    return occlusion;
}

vec3 calculateLight(Light light, vec3 fragPos, vec3 n, vec3 v, vec3 cameraPos,
        vec3 diffuseColor, float roughness, float metallic, vec3 f0) {
    vec3 l = light.direction;
    if (light.type != TYPE_DIRECTIONAL_LIGHT) {
        l = normalize(light.position - fragPos);
    }
    vec3 h = normalize(v + l);
    float NoL = clamp(dot(n, l), 0.0, 1.0);

    float occlusion = calculateOcclusion(light, fragPos, cameraPos, NoL);

    float atten = 1.0;
    if (light.type != TYPE_DIRECTIONAL_LIGHT) {
        atten = calculateAttenuation(fragPos, l, light);
    }

#ifdef PBR
    vec3 fr = pbrBRDF(
            diffuseColor, roughness, metallic, f0,
            n, v, l, h);
    // TODO: figure out how to properly compute light intensity for PBR
    light.intensity *= 2.6;
#else
    vec3 fr = blinnPhongBRDF(diffuseColor, n, v, l, h);
#endif
    return (fr * light.color) * (light.intensity * atten * NoL * occlusion);
}

#endif // LIGHT_H
