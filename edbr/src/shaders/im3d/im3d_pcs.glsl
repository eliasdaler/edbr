#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout: require

struct VertexFormat {
    vec4 positionSize;
    uint color;
};

layout (buffer_reference, scalar) readonly buffer VertexBuffer {
	VertexFormat vertices[];
};

layout (push_constant, scalar) uniform constants
{
    VertexBuffer vertices;
    mat4 uViewProjMatrix;
    vec2 uViewport;
} pcs;

#define VertexData \
	_VertexData { \
		noperspective float m_edgeDistance; \
		noperspective float m_size; \
		smooth vec4 m_color; \
	}

#define kAntialiasing 2.0

