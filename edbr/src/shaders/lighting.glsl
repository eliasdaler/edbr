#ifndef LIGHTING_H
#define LIGHTING_H

#include "light.glsl"

#include "pbr.glsl"
#include "blinn_phong.glsl"

#include "shadow.glsl"

#define PBR

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

vec3 calculateLight(Light light, vec3 fragPos, vec3 n, vec3 v, vec3 l,
        vec3 diffuseColor, float roughness, float metallic, vec3 f0, float occlusion) {
    vec3 h = normalize(v + l);
    float NoL = clamp(dot(n, l), 0.0, 1.0);

    float atten = 1.0;
    if (light.type != TYPE_DIRECTIONAL_LIGHT) {
        atten = calculateAttenuation(fragPos, l, light);
    }

#ifdef PBR
    vec3 fr = pbrBRDF(
            diffuseColor, roughness, metallic, f0,
            n, v, l, h, NoL);
    // TODO: figure out how to properly compute light intensity for PBR
    light.intensity *= 2.6;
#else
    vec3 fr = blinnPhongBRDF(diffuseColor, n, v, l, h);
#endif
    return (fr * light.color) * (light.intensity * atten * NoL * occlusion);
}

#endif // LIGHTING_H
