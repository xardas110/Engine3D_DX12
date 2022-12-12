#ifndef RAYTRACING_HELPER_H
#define RAYTRACING_HELPER_H

#define hlsl 
#include "TypesCompat.h"

struct HitAttributes
{
    float2 barycentrics;
    int primitiveIndex;
    int geometryIndex;
    float3x4 objToWorld;
    int instanceIndex;
    float minT;
    bool bFrontFaced;
    bool bHit;
};

struct RayInfo
{
    RayDesc desc;
    uint instanceMask;
    uint flags;
};

bool CastRay(RayInfo ray, RaytracingAccelerationStructure scene, inout HitAttributes hit)
{
    RayQuery < RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES > query;
    
    query.TraceRayInline(scene, ray.flags, ray.instanceMask, ray.desc);
    
    while (query.Proceed())
    {
    };
  
    hit.bFrontFaced = query.CommittedTriangleFrontFace();
    hit.instanceIndex = query.CommittedInstanceID();
    hit.primitiveIndex = query.CommittedPrimitiveIndex();
    hit.geometryIndex = query.CommittedGeometryIndex();
    hit.barycentrics = query.CommittedTriangleBarycentrics();
    hit.objToWorld = query.CommittedObjectToWorld3x4();
    hit.minT = query.CommittedRayT();
    
    return query.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
}

bool TraceVisibility(RayInfo ray, RaytracingAccelerationStructure scene)
{
    RayQuery < RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > shadowQuery;    
    shadowQuery.TraceRayInline(scene, ray.flags, ray.instanceMask, ray.desc);
    shadowQuery.Proceed();
    
    return shadowQuery.CommittedStatus() != COMMITTED_TRIANGLE_HIT;
}

float2 BaryInterp2(in float2 v0, in float2 v1, in float2 v2, in float3 bary)
{
    return bary.x * v0 + bary.y * v1 + bary.z * v2;;
}

float3 BaryInterp3(in float3 v0, in float3 v1, in float3 v2, in float3 bary)
{
    //X = (u * p0.x + v * p1.x + w * p2.x)
    //Y = (u * p0.y + v * p1.y + w * p2.y)
    //Z = (u * p0.z + v * p1.z + w * p2.z)
    
    return bary.x * v0 + bary.y * v1 + bary.z * v2;
}

MeshVertex BarycentricLerp(in MeshVertex v0, in MeshVertex v1, in MeshVertex v2, in float3 bary)
{
    MeshVertex r;
    r.position = BaryInterp3(v0.position, v1.position, v2.position, bary);
    r.normal = BaryInterp3(v0.normal, v1.normal, v2.normal, bary);
    r.textureCoordinate = BaryInterp2(v0.textureCoordinate, v1.textureCoordinate, v2.textureCoordinate, bary);
    r.tangent = BaryInterp3(v0.tangent, v1.tangent, v2.tangent, bary);
    r.bitangent = BaryInterp3(v0.bitangent, v1.bitangent, v2.bitangent, bary);
    return r;
}

//In object space
MeshVertex GetHitSurface(in HitAttributes attr, in MeshInfo meshInfo, in StructuredBuffer<MeshVertex> globalMeshVertexData[], in StructuredBuffer<uint> globalMeshIndexData[])
{
    float3 bary;
    bary.x = 1 - attr.barycentrics.x - attr.barycentrics.y;
    bary.y = attr.barycentrics.x;
    bary.z = attr.barycentrics.y;
    
    StructuredBuffer<MeshVertex> vertexBuffer = globalMeshVertexData[meshInfo.vertexOffset];
    StructuredBuffer<uint> indexBuffer = globalMeshIndexData[meshInfo.indexOffset];

    const uint primId = attr.primitiveIndex;
    const uint i0 = indexBuffer[primId * 3 + 0];
    const uint i1 = indexBuffer[primId * 3 + 1];
    const uint i2 = indexBuffer[primId * 3 + 2];
   
    MeshVertex v0 = vertexBuffer[i0];
    MeshVertex v1 = vertexBuffer[i1];
    MeshVertex v2 = vertexBuffer[i2];
    
    MeshVertex result = BarycentricLerp(v0, v1, v2, bary);
    return result;
}


void GetCameraDirectionFromUV(in uint2 index, in float2 resolution, in float3 camPos, in float4x4 invViewProj, out float3 direction)
{
    float2 xy = index + 0.5f;
    float2 screenPos = xy / resolution * 2.f - 1.f;
    // Invert Y for DirectX-style coordinates
    screenPos.y = -screenPos.y;

    // Unproject into a ray
    float4 unprojected = mul(invViewProj, float4(screenPos, 0, 1));
    float3 world = unprojected.xyz / unprojected.w;
    direction = normalize(world - camPos);
}

#endif