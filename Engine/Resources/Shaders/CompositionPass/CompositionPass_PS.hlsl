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

struct PixelShaderOutput
{
    float4 ColorTexture    : SV_TARGET0;
};

PixelShaderOutput main(float2 TexCoord : TEXCOORD)
{
    PixelShaderOutput OUT;

    float4 directDiffuse = g_LightMapHeap[0].Sample(g_NearestRepeatSampler, TexCoord);
    float4 indirectDiffuse = g_LightMapHeap[7].Sample(g_NearestRepeatSampler, TexCoord);
    float4 indirectSpecular = g_LightMapHeap[8].Sample(g_NearestRepeatSampler, TexCoord);
   
    indirectDiffuse = REBLUR_BackEnd_UnpackRadianceAndNormHitDist(indirectDiffuse);
    indirectSpecular = REBLUR_BackEnd_UnpackRadianceAndNormHitDist(indirectSpecular);
         
    float4 albedo = g_GBufferHeap[0].Sample(g_NearestRepeatSampler, TexCoord);
    float4 normal = g_GBufferHeap[1].Sample(g_NearestRepeatSampler, TexCoord);
    float4 pbr = g_GBufferHeap[2].Sample(g_NearestRepeatSampler, TexCoord);
    float4 emissive = g_GBufferHeap[3].Sample(g_NearestRepeatSampler, TexCoord);
    
        OUT.ColorTexture = float4(directDiffuse.rgb + indirectDiffuse.rgb + indirectSpecular.rgb, 1.f);

    return OUT;
}