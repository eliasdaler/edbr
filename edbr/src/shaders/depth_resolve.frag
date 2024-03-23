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

    // We want to do max and not min here because sometimes it's possible to
    // get "holes" between flat surfaces e.g. tiles and min(depth) reveals
    // them, but there's no such problems with using max depth for resolve.
    // (note that reverse depth is used, to "far" is 0.0 and not 1.0)
    float depth = 0.0;
    for (int i = 0; i < pcs.numSamples; ++i) {
        depth = max(depth, sampleTexture2DMSNearest(pcs.depthImageId, pixel, i).r);
    }
    gl_FragDepth = depth;
}
