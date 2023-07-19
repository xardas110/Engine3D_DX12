#pragma once
#define hlsl
#include "../Common/TypesCompat.h"
#include "../Common/MaterialAttributes.hlsl"

#include "..\..\..\Libs\RayTracingDenoiser\Shaders\Include\NRDEncoding.hlsli"

#define NRD_HEADER_ONLY
#include "..\..\..\Libs\RayTracingDenoiser\Shaders\Include\NRD.hlsli"

// GBuffer Structures
struct GFragment
{
    float3 albedo;
    float ao;
    float3 normal;
    float height;
    float3 emissive;
    float depth;
    float3 motionVector;
    float linearDepth;
    float roughness;
    float metallic;
    int shaderModel;
    float3 geometryNormal;
};

struct GPackInfo
{
    float4 albedo;
    float4 normalRough;
    float4 motionVector;
    float4 emissiveSM;
    float4 aoMetallicHeight;
    float4 geometryNormal;
    float linearDepth;
};

GPackInfo PackGBuffer(in CameraCB camera, in ObjectCB objectCB, in SurfaceMaterial mat, in float3 fragPos, float depth, in float3 geometryNormal)
{
    GPackInfo info;

    info.albedo = float4(mat.albedo, 1.f);
    info.motionVector = float4(0.f, 0.f, 0.f, 0.f);  // Placeholder for Motion Vector
    info.normalRough = NRD_FrontEnd_PackNormalAndRoughness(mat.normal, mat.roughness);
    info.emissiveSM = float4(mat.emissive, float(objectCB.shaderModel) / 255.f);
    info.aoMetallicHeight = float4(mat.ao, mat.metallic, mat.height, 0.f);
    info.linearDepth = depth;
    info.geometryNormal = float4(geometryNormal, 1.f);

    return info;
}

GFragment UnpackGBuffer(in Texture2D gBufferHeap[], in SamplerState nearestSampler, in float2 texCoords, int lod = 0)
{
    GFragment frag;

    float4 albedoTex = gBufferHeap[GBUFFER_ALBEDO].Sample(nearestSampler, texCoords, lod);
    float4 normalRoughTex = NRD_FrontEnd_UnpackNormalAndRoughness(gBufferHeap[GBUFFER_NORMAL_ROUGHNESS].Sample(nearestSampler, texCoords, lod));
    float4 motionVector = gBufferHeap[GBUFFER_MOTION_VECTOR].Sample(nearestSampler, texCoords, lod);
    float4 emissiveSMTex = gBufferHeap[GBUFFER_EMISSIVE_SHADER_MODEL].Sample(nearestSampler, texCoords, lod);
    float4 aoMetallicHeight = gBufferHeap[GBUFFER_AO_METALLIC_HEIGHT].Sample(nearestSampler, texCoords, lod);
    float4 geometryNormal = gBufferHeap[GBUFFER_GEOMETRY_NORMAL].Sample(nearestSampler, texCoords, lod);
    float linearDepth = gBufferHeap[GBUFFER_LINEAR_DEPTH].Sample(nearestSampler, texCoords, lod).r;
    float standardDepth = gBufferHeap[GBUFFER_STANDARD_DEPTH].Sample(nearestSampler, texCoords, lod).r;

    frag.albedo = albedoTex.rgb;
    frag.normal = normalize(normalRoughTex.rgb);
    frag.roughness = normalRoughTex.w;
    frag.linearDepth = linearDepth;
    frag.ao = aoMetallicHeight.x;
    frag.emissive = emissiveSMTex.rgb;
    frag.shaderModel = emissiveSMTex.w * 255.f;
    frag.motionVector = motionVector.rgb;
    frag.metallic = aoMetallicHeight.y;
    frag.height = aoMetallicHeight.z;
    frag.depth = standardDepth;
    frag.geometryNormal = geometryNormal.rgb;

    return frag;
}