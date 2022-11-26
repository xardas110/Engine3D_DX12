#define HLSL
#include "../Common/TypesCompat.h"

#include "..\..\..\Libs\RayTracingDenoiser\Shaders\Include\NRDEncoding.hlsli"

#define NRD_HEADER_ONLY
#include "..\..\..\Libs\RayTracingDenoiser\Shaders\Include\NRD.hlsli"

RaytracingAccelerationStructure g_Scene                     : register(t0);
Texture2D                       g_GlobalTextureData[]       : register(t1, space0);
StructuredBuffer<MeshVertex>    g_GlobalMeshVertexData[]    : register(t1, space1);
StructuredBuffer<uint>          g_GlobalMeshIndexData[]     : register(t1, space2);

StructuredBuffer<MeshInfo>      g_GlobalMeshInfo            : register(t2, space3);
StructuredBuffer<MaterialInfo>  g_GlobalMaterialInfo        : register(t3, space4);
StructuredBuffer<Material>      g_GlobalMaterials           : register(t4, space5);

Texture2D                       g_GBufferHeap[]             : register(t5, space6);
Texture2D                       g_LightMapHeap[]            : register(t6, space7);

SamplerState                    g_NearestRepeatSampler      : register(s0);
SamplerState                    g_LinearRepeatSampler       : register(s1);

ConstantBuffer<RaytracingDataCB> g_RaytracingData           : register(b0);
    
struct PixelShaderOutput
{
    float4 ColorTexture    : SV_TARGET0;
};

PixelShaderOutput main(float2 TexCoord : TEXCOORD)
{
    PixelShaderOutput OUT;

    float4 directDiffuse = g_LightMapHeap[LIGHTBUFFER_DIRECT_LIGHT].Sample(g_NearestRepeatSampler, TexCoord);
    float4 denoisedIndirectDiffuse = g_LightMapHeap[LIGHTBUFFER_DENOISED_INDIRECT_DIFFUSE].Sample(g_NearestRepeatSampler, TexCoord);
    float4 denoisedIndirectSpecular = g_LightMapHeap[LIGHTBUFFER_DENOISED_INDIRECT_SPECULAR].Sample(g_NearestRepeatSampler, TexCoord);
   
    denoisedIndirectDiffuse = REBLUR_BackEnd_UnpackRadianceAndNormHitDist(denoisedIndirectDiffuse);
    denoisedIndirectSpecular = REBLUR_BackEnd_UnpackRadianceAndNormHitDist(denoisedIndirectSpecular);
         
    float4 albedo = g_GBufferHeap[GBUFFER_ALBEDO].Sample(g_NearestRepeatSampler, TexCoord);
 
    OUT.ColorTexture = float4(directDiffuse.rgb + denoisedIndirectDiffuse.rgb + denoisedIndirectSpecular.rgb, 1.f);

    if (g_RaytracingData.debugSettings == DEBUG_LIGHTBUFFER_DENOISED_INDIRECT_DIFFUSE)
    {
        OUT.ColorTexture = float4(denoisedIndirectDiffuse.rgb, 1.f);
    }
    else if (g_RaytracingData.debugSettings == DEBUG_LIGHTBUFFER_DENOISED_INDIRECT_SPECULAR)
    {
         OUT.ColorTexture = float4(denoisedIndirectSpecular.rgb, 1.f);   
    }
    return OUT;
}