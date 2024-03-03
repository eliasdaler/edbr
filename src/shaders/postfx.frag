#version 460

layout (location = 0) in vec2 inUV;

layout (set = 0, binding = 0) uniform sampler2D drawImage;

layout (location = 0) out vec4 outFragColor;

vec3 chromaticAberration(sampler2D tex, vec2 uv, float amount) {
    vec3 col;
    col.r = texture(tex, vec2(uv.x+amount, uv.y)).r;
    col.g = texture(tex, uv).g;
    col.b = texture(tex, vec2(uv.x-amount, uv.y)).b;
    col *= (1.0 - amount * 0.5);
    return col;
}

void main() {
    vec3 color = chromaticAberration(drawImage, inUV, 0.001f);
    outFragColor = vec4(color, 1.0);
}
