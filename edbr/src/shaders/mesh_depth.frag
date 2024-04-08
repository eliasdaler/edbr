#version 460

#extension GL_GOOGLE_include_directive : require

#include "bindless.glsl"
#include "vertex.glsl"
#include "materials.glsl"

layout (push_constant) uniform constants
{
	mat4 mvp;
	VertexBuffer vertexBuffer;
    MaterialsBuffer materials;
    uint materialID;
} pcs;

layout (location = 0) in vec2 inUV;

void main()
{
    MaterialData material = pcs.materials.data[pcs.materialID];

    vec4 diffuse = sampleTexture2DLinear(material.diffuseTex, inUV);
    if (diffuse.a < 0.1) {
        discard;
    }
}

