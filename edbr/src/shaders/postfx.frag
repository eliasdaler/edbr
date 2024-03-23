#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_scalar_block_layout: require

layout (location = 0) in vec2 inUV;

#include "bindless.glsl"
#include "scene_data.glsl"

layout (push_constant, scalar) uniform constants
{
    SceneDataBuffer sceneData;
    uint drawImage;
    uint depthImage;
} pcs;

layout (location = 0) out vec4 outFragColor;

vec3 getViewPos(float depth, mat4 invProj, vec2 uv) {
    vec4 clip = invProj * vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec3 pos = clip.xyz / clip.w;
    return pos;
}

vec3 exponentialFog(vec3 pos, vec3 color,
        vec3 fogColor, float fogDensity,
        vec3 ambientColor, float ambientIntensity,
        vec3 dirLightColor) {
    vec3 fc = fogColor * (ambientColor * ambientIntensity + dirLightColor);
    float dist = length(pos);
    float fogFactor = 1.0 / exp((dist * fogDensity) * (dist * fogDensity));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    return mix(fc, color, fogFactor);
}

vec3 chromaticAberration(sampler2D tex, vec2 uv, float amount) {
    vec3 col;
    col.r = texture(tex, vec2(uv.x+amount, uv.y)).r;
    col.g = texture(tex, uv).g;
    col.b = texture(tex, vec2(uv.x-amount, uv.y)).b;
    col *= (1.0 - amount * 0.5);
    return col;
}

void main() {
    vec3 fragColor = sampleTexture2DNearest(pcs.drawImage, inUV).rgb;
    float depth = sampleTexture2DNearest(pcs.depthImage, inUV).r;

    vec3 sunlightColor = vec3(0, 0, 0);
    if (pcs.sceneData.sunlightIndex != -1) {
        sunlightColor = pcs.sceneData.lights.data[pcs.sceneData.sunlightIndex].color;
    }

    vec3 viewPos = getViewPos(depth, inverse(pcs.sceneData.proj), inUV);
    vec3 color = exponentialFog(viewPos, fragColor,
            pcs.sceneData.fogColor, pcs.sceneData.fogDensity,
            pcs.sceneData.ambientColor, pcs.sceneData.ambientIntensity,
            sunlightColor);
    outFragColor = vec4(color, 1.0);

    // vec3 color = chromaticAberration(drawImage, inUV, 0.001f);
    // outFragColor = vec4(color, 1.0);
}
