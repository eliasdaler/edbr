#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout: require

struct VertexFormat {
  vec2 position;
  vec2 uv;
  uint color;
};

layout (buffer_reference, scalar) readonly buffer VertexBuffer {
	VertexFormat vertices[];
};

layout (push_constant, scalar) uniform constants
{
  VertexBuffer vertexBuffer;
  uint textureId;
  uint textureIsSRGB;
  vec2 translate;
  vec2 scale;
} pcs;

