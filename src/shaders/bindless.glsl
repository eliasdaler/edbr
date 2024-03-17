#extension GL_EXT_nonuniform_qualifier : enable

layout (set = 0, binding = 0) uniform texture2D textures[];
layout (set = 0, binding = 0) uniform textureCube textureCubes[];
layout (set = 0, binding = 0) uniform texture2DArray textureArrays[];
layout (set = 0, binding = 1) uniform sampler samplers[];

#define NEAREST_SAMPLER_ID 0
#define LINEAR_SAMPLER_ID  1
#define SHADOW_SAMPLER_ID  2

vec4 sampleTexture2DNearest(uint texID, vec2 uv) {
    return texture(nonuniformEXT(sampler2D(textures[texID], samplers[NEAREST_SAMPLER_ID])), uv);
}

vec4 sampleTexture2DLinear(uint texID, vec2 uv) {
    return texture(nonuniformEXT(sampler2D(textures[texID], samplers[LINEAR_SAMPLER_ID])), uv);
}

vec4 sampleTextureCubeLinear(uint texID, vec3 p) {
    return texture(nonuniformEXT(samplerCube(textureCubes[texID], samplers[NEAREST_SAMPLER_ID])), p);
}

float sampleTextureArrayShadow(uint texID, vec4 p) {
    return texture(nonuniformEXT(sampler2DArrayShadow(textureArrays[texID], samplers[SHADOW_SAMPLER_ID])), p);
}
