layout (set = 1, binding = 0) uniform MaterialData {
    vec4 baseColor;
    vec4 metallicRoughnessEmissive;
} materialData;

layout (set = 1, binding = 1) uniform sampler2D diffuseTex;
layout (set = 1, binding = 2) uniform sampler2D normalMapTex;
layout (set = 1, binding = 3) uniform sampler2D metallicRoughnessTex;
layout (set = 1, binding = 4) uniform sampler2D emissiveTex;
