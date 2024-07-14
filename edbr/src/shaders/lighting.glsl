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
#else
    vec3 fr = blinnPhongBRDF(diffuseColor, n, v, l, h);
#endif

#ifndef PBR
    light.intensity *= 0.2;
#endif

    return (fr * light.color) * (light.intensity * atten * NoL * occlusion);
}

vec3 calculateBumpMapNormal(uint bumpMapTex, vec3 pos, vec3 normal, vec2 uv) {
    // Bump Mapping Unparametrized Surfaces on the GPU
    // https://mmikk.github.io/papers3d/mm_sfgrad_bump.pdf
    vec3 posDX = dFdxFine(pos.xyz);
    vec3 posDY = dFdyFine(pos.xyz);
    vec3 r1 = cross(posDY, normal);
    vec3 r2 = cross(normal, posDX);
    float det = dot(posDX, r1);

    // height from bump map texture
    float Hll = sampleTexture2DLinear(bumpMapTex, uv ).x;
    float Hlr = sampleTexture2DLinear(bumpMapTex, uv + dFdx(uv.xy)).x;
    float Hul = sampleTexture2DLinear(bumpMapTex, uv + dFdy(uv.xy)).x;
    // float dBs = ddx_fine ( height );     // optional explicit height
    // float dBt = ddy_fine ( height );

    // gradient of surface texture. dBs=Hlr-Hll, dBt=Hul-Hll
    vec3 surf_grad = sign(det) * ((Hlr - Hll) * r1 + (Hul - Hll)* r2);
    float bump_amt = 0.3;   // adjustable bump amount
    return normal * (1.0-bump_amt) +
        bump_amt * normalize (abs(det)*normal - surf_grad);
}

#endif // LIGHTING_H
