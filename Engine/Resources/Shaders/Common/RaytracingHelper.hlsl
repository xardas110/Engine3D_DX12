#ifndef RAYTRACING_HELPER_H
#define RAYTRACING_HELPER_H

#define hlsl 
#include "TypesCompat.h"
#include "MaterialAttributes.hlsl"

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
    uint anyhitFlags;
};

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

/////////// Begin ray cone functions  /////////// 
//Raytracing gems chapter 7
uint2 TexDims(Texture2D<float4> tex)
{
    uint2 vSize;
    tex.GetDimensions(vSize.x, vSize.y);
    return vSize;
}
uint2 TexDims(Texture2D<float3> tex)
{
    uint2 vSize;
    tex.GetDimensions(vSize.x, vSize.y);
    return vSize;
}
uint2 TexDims(Texture2D<float> tex)
{
    uint2 vSize;
    tex.GetDimensions(vSize.x, vSize.y);
    return vSize;
}

//Raytracing gems chapter 7
float2 UVAreaFromRayCone(float3 vRayDir, float3 vWorldNormal, float vRayConeWidth, float2 aUV[3], float3 aPos[3], float3x3 matWorld)
{
    float2 vUV10 = aUV[1] - aUV[0];
    float2 vUV20 = aUV[2] - aUV[0];
    float fTriUVArea = abs(vUV10.x * vUV20.y - vUV20.x * vUV10.y);

	// We need the area of the triangle, which is length(triangleNormal) in worldspace, and I
	// could not figure out a way with fewer than two 3x3 mtx multiplies for ray cones.
    float3 vEdge10 = mul(aPos[1] - aPos[0], matWorld);
    float3 vEdge20 = mul(aPos[2] - aPos[0], matWorld);

    float3 vFaceNrm = cross(vEdge10, vEdge20); // in world space, by design
    float fTriLODOffset = 0.5f * log2(fTriUVArea / length(vFaceNrm)); // Triangle-wide LOD offset value
    float fDistTerm = vRayConeWidth * vRayConeWidth;
    float fNormalTerm = dot(vRayDir, vWorldNormal);

    return float2(fTriLODOffset, fDistTerm / (fNormalTerm * fNormalTerm));
}

float UVAreaToTexLOD(uint2 vTexSize, float2 vUVAreaInfo)
{
    return vUVAreaInfo.x + 0.5f * log2(vTexSize.x * vTexSize.y * vUVAreaInfo.y);
}

float4 UVDerivsFromRayCone(float3 vRayDir, float3 vWorldNormal, float vRayConeWidth, float2 aUV[3], float3 aPos[3], float3x3 matWorld)
{
    float2 vUV10 = aUV[1] - aUV[0];
    float2 vUV20 = aUV[2] - aUV[0];
    float fQuadUVArea = abs(vUV10.x * vUV20.y - vUV20.x * vUV10.y);

	// Since the ray cone's width is in world-space, we need to compute the quad
	// area in world-space as well to enable proper ratio calculation
    float3 vEdge10 = mul(aPos[1] - aPos[0], matWorld);
    float3 vEdge20 = mul(aPos[2] - aPos[0], matWorld);
    float3 vFaceNrm = cross(vEdge10, vEdge20);
    float fQuadArea = length(vFaceNrm);

    float fDistTerm = abs(vRayConeWidth);
    float fNormalTerm = abs(dot(vRayDir, vWorldNormal));
    float fProjectedConeWidth = vRayConeWidth / fNormalTerm;
    float fVisibleAreaRatio = (fProjectedConeWidth * fProjectedConeWidth) / fQuadArea;

    float fVisibleUVArea = fQuadUVArea * fVisibleAreaRatio;
    float fULength = sqrt(fVisibleUVArea);
    return float4(fULength, 0, 0, fULength);
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