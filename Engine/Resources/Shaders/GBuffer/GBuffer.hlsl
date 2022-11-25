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
};

//Tighterpacking later, once stuff is working
struct GPackInfo
{
    float4 albedo;
    float4 normalRough;
    float4 motionVector;
    float4 emissiveSM;
    float4 aoMetallicHeight;
    float linearDepth;
};

GPackInfo PackGBuffer(in CameraCB camera, in ObjectCB objectCB, in SurfaceMaterial mat, in float3 fragPos, float depth)
{
    GPackInfo OUT;
    
    OUT.albedo = float4(mat.albedo, 1.f);
    
    //TODO: CALC MOTION VECTOR
    OUT.motionVector = float4(0.f, 0.f, 0.f, 0.f);
    
    OUT.normalRough = NRD_FrontEnd_PackNormalAndRoughness(mat.normal, mat.roughness);
    
    OUT.emissiveSM = float4(mat.emissive, float(objectCB.shaderModel) / 255.f);
    
    OUT.aoMetallicHeight = float4(mat.ao, mat.metallic, mat.height, 0.f);
    
    OUT.linearDepth = (camera.zNear * camera.zFar) / (camera.zFar + camera.zNear - depth * (camera.zFar - camera.zNear));
        
    return OUT;
}

//Temp, until better packing is added
GFragment UnpackGBuffer(in Texture2D gBufferHeap[], 
    in SamplerState nearestSampler, 
    in float2 texCoords)
{
    GFragment fi;
    
    float4 albedoTex = gBufferHeap[GBUFFER_ALBEDO].Sample(nearestSampler, texCoords);
    
    float4 normalRoughTex = NRD_FrontEnd_UnpackNormalAndRoughness(gBufferHeap[GBUFFER_NORMAL_ROUGHNESS].Sample(nearestSampler, texCoords));
    
    float4 motionVector = gBufferHeap[GBUFFER_MOTION_VECTOR].Sample(nearestSampler, texCoords);
    
    float4 emissiveSMTex = gBufferHeap[GBUFFER_EMISSIVE_SHADER_MODEL].Sample(nearestSampler, texCoords);
    
    float4 aoMetallicHeight = gBufferHeap[GBUFFER_AO_METALLIC_HEIGHT].Sample(nearestSampler, texCoords);
    
    float linearDepth = gBufferHeap[GBUFFER_LINEAR_DEPTH].Sample(nearestSampler, texCoords).r;
    
    float standardDepth = gBufferHeap[GBUFFER_STANDARD_DEPTH].Sample(nearestSampler, texCoords).r;
    
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
    
    return fi;
}