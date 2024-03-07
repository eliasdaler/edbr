#version 460

layout (location = 0) in vec2 inUV;

layout (set = 0, binding = 0) uniform sampler2DMS depthImage;

layout (push_constant) uniform constants
{
	vec4 screenSizeAndNumSamples;
} pushConstants;

void main() {
    vec2 screenSize = pushConstants.screenSizeAndNumSamples.xy;
    int numSamples = int(pushConstants.screenSizeAndNumSamples.z);

    ivec2 pixel = ivec2(inUV * screenSize);

    // We want to do max and not min here because sometimes it's possible to
    // get "holes" between flat surfaces e.g. tiles and min(depth) reveals
    // them, but there's no such problems with using max depth for resolve.
    // (note that reverse depth is used, to "far" is 0.0 and not 1.0)
    float depth = 0.0;
    for (int i = 0; i < numSamples; ++i) {
        depth = max(depth, texelFetch(depthImage, pixel, i).r);
    }
    gl_FragDepth = depth;
}
