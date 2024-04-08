#version 460

#extension GL_GOOGLE_include_directive : require

#include "bindless.glsl"
#include "mesh_pcs.glsl"

layout (location = 0) in vec2 inUV;

void main()
{
    MaterialData material = pcs.sceneData.materials.data[pcs.materialID];

    vec4 diffuse = sampleTexture2DLinear(material.diffuseTex, inUV);
    if (diffuse.a < 0.1) {
        discard;
    }
}

