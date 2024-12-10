#version 460

#extension GL_GOOGLE_include_directive : require

#include "mesh_pcs.glsl"

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec4 outTangent;
layout (location = 4) out mat3 outTBN;

mat3 adjoint(in mat4 m)
{
    return mat3(cross(m[1].xyz, m[2].xyz),
                cross(m[2].xyz, m[0].xyz),
                cross(m[0].xyz, m[1].xyz));
}

void main()
{
    Vertex v = pcs.vertexBuffer.vertices[gl_VertexIndex];

    vec4 worldPos = pcs.transform * vec4(v.position, 1.0f);

    gl_Position = pcs.sceneData.viewProj * worldPos;
    outPos = worldPos.xyz;
    outUV = vec2(v.uv_x, v.uv_y);

    // Simpler case, when everything is uniform:
    //     outNormal = (pcs.transform * vec4(v.normal, 0.0)).xyz;
    // We're using adjoint it instead of doing:
    //     outNormal = mat3(transpose(inverse(pcs.transform))) * v.normal;
    // See https://github.com/graphitemaster/normals_revisited
    outNormal = adjoint(pcs.transform) * v.normal;

    outTangent = v.tangent;

    vec3 T = normalize(vec3(pcs.transform * v.tangent));
    vec3 N = normalize(outNormal);
    vec3 B = cross(N, T) * v.tangent.w;
    outTBN = mat3(T, B, N);
}
