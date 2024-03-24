#version 460

#extension GL_GOOGLE_include_directive : require

#include "imgui_pcs.glsl"

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outUV;

void main() {
  const VertexFormat v = pcs.vertexBuffer.vertices[gl_VertexIndex];
  const vec4 color = unpackUnorm4x8(v.color);
  outColor = color;
  outUV = v.uv;

  gl_Position = vec4(v.position * pcs.scale + pcs.translate, 0.0, 1.0);
}
