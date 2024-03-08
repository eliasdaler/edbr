layout (set = 1, binding = 0) uniform MaterialData {
    vec4 baseColor;
    vec4 metallicRoughness;
} materialData;

layout (set = 1, binding = 1) uniform sampler2D diffuseTex;
