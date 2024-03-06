#version 460

layout (location = 0) in vec2 inUV;

layout (push_constant) uniform constants
{
	mat4 invProj;
    vec4 fogColorAndDensity; // (color.rgb, density)
    vec4 ambientColorAndIntensity; // (color.rgb, intensity)
    vec4 sunlightColorAndIntensity; // (color.rgb, intensity)
} pcs;

layout (set = 0, binding = 0) uniform sampler2D drawImage;
layout (set = 0, binding = 1) uniform sampler2D depthImage;

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
    vec3 fragColor = texture(drawImage, inUV).rgb;
    float depth = texture(depthImage, inUV).r;

    vec3 viewPos = getViewPos(depth, pcs.invProj, inUV);
    vec3 color = exponentialFog(viewPos, fragColor,
            pcs.fogColorAndDensity.rgb, pcs.fogColorAndDensity.w,
            pcs.ambientColorAndIntensity.rgb, pcs.ambientColorAndIntensity.w,
            pcs.sunlightColorAndIntensity.rgb);
    outFragColor = vec4(color, 1.0);

    // vec3 color = chromaticAberration(drawImage, inUV, 0.001f);
    // outFragColor = vec4(color, 1.0);
}
