#pragma once
#define hlsl
#include "../Common/TypesCompat.h"
#include "../Common/MaterialAttributes.hlsl"

#include "..\..\..\Libs\RayTracingDenoiser\Shaders\Include\NRDEncoding.hlsli"

#define NRD_HEADER_ONLY
#include "..\..\..\Libs\RayTracingDenoiser\Shaders\Include\NRD.hlsli"

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

//Tighterpacking later, once stuff is working
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
    GPackInfo OUT;
    
    OUT.albedo = float4(mat.albedo, 1.f);
    
    //TODO: CALC MOTION VECTOR
    OUT.motionVector = float4(0.f, 0.f, 0.f, 0.f);
    
    OUT.normalRough = NRD_FrontEnd_PackNormalAndRoughness(mat.normal, mat.roughness);
    
    OUT.emissiveSM = float4(mat.emissive, float(objectCB.shaderModel) / 255.f);
    
    OUT.aoMetallicHeight = float4(mat.ao, mat.metallic, mat.height, 0.f);
    
    OUT.linearDepth = depth;
        
    OUT.geometryNormal = float4(geometryNormal, 1.f);
    
    return OUT;
}

//Temp, until better packing is added
GFragment UnpackGBuffer(in Texture2D gBufferHeap[], 
    in SamplerState nearestSampler, 
    in float2 texCoords)
{
    GFragment fi;
    
    float4 albedoTex = gBufferHeap[GBUFFER_ALBEDO].Sample(nearestSampler, texCoords, 0.f);
    
    float4 normalRoughTex = NRD_FrontEnd_UnpackNormalAndRoughness(gBufferHeap[GBUFFER_NORMAL_ROUGHNESS].Sample(nearestSampler, texCoords, 0.f));
    
    float4 motionVector = gBufferHeap[GBUFFER_MOTION_VECTOR].Sample(nearestSampler, texCoords, 0.f);
    
    float4 emissiveSMTex = gBufferHeap[GBUFFER_EMISSIVE_SHADER_MODEL].Sample(nearestSampler, texCoords, 0.f);
    
    float4 aoMetallicHeight = gBufferHeap[GBUFFER_AO_METALLIC_HEIGHT].Sample(nearestSampler, texCoords, 0.f);
    
    float4 geometryNormal = gBufferHeap[GBUFFER_GEOMETRY_NORMAL].Sample(nearestSampler, texCoords, 0.f);
    
    float linearDepth = gBufferHeap[GBUFFER_LINEAR_DEPTH].Sample(nearestSampler, texCoords, 0.f).r;
    
    float standardDepth = gBufferHeap[GBUFFER_STANDARD_DEPTH].Sample(nearestSampler, texCoords, 0.f).r;
    
    fi.albedo = albedoTex.rgb;
    
    fi.normal = normalize(normalRoughTex.rgb);
    
    fi.roughness = normalRoughTex.w;
    
    fi.linearDepth = linearDepth;
    
    fi.ao = aoMetallicHeight.x;
    
    fi.emissive = emissiveSMTex.rgb;
    
    fi.shaderModel = emissiveSMTex.w * 255.f;
    
    fi.motionVector = motionVector.rgb;
    
    fi.metallic = aoMetallicHeight.y;
    
    fi.height = aoMetallicHeight.z;
    
    fi.depth = standardDepth;
    
    fi.geometryNormal = geometryNormal.rgb;
    
    return fi;
}