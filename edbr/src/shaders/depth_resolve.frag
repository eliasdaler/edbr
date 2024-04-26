#version 460

#extension GL_EXT_scalar_block_layout: require

layout (location = 0) in vec2 inUV;

#include "bindless.glsl"

layout (push_constant, scalar) uniform constants
{
    vec2 depthImageSize;
    uint depthImageId;
    int numSamples;
} pcs;

void main() {
    ivec2 pixel = ivec2(inUV * pcs.depthImageSize);

    float depth = 1.0;
    for (int i = 0; i < pcs.numSamples; ++i) {
        depth = min(depth, sampleTexture2DMSNearest(pcs.depthImageId, pixel, i).r);
    }
    gl_FragDepth = depth;
}
