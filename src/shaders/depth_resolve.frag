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
    float minDepth = 1.0;
    for (int i = 0; i < numSamples; ++i) {
        minDepth = min(minDepth, texelFetch(depthImage, pixel, i).r);
    }
    gl_FragDepth = minDepth;
}
