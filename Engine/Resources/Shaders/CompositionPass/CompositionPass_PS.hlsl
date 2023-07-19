#define HLSL
#include "../Common/TypesCompat.h"

#include "..\..\..\Libs\RayTracingDenoiser\Shaders\Include\NRDEncoding.hlsli"

#define NRD_HEADER_ONLY
#include "..\..\..\Libs\RayTracingDenoiser\Shaders\Include\NRD.hlsli"

#include "../Common/BRDF.hlsl"
#include "../GBuffer/GBuffer.hlsl"
#include "../Common/MaterialAttributes.hlsl"
#include "../Common/RaytracingHelper.hlsl"
#include "../Common/HDR.hlsl"

// Resource declarations
RaytracingAccelerationStructure     g_Scene                : register(t0);
Texture2D                           g_GlobalTextureData[]  : register(t1, space0);
StructuredBuffer<MeshVertex>        g_GlobalMeshVertexData : register(t1, space1);
StructuredBuffer<uint>              g_GlobalMeshIndexData  : register(t1, space2);
StructuredBuffer<MeshInfo>          g_GlobalMeshInfo       : register(t2, space3);
StructuredBuffer<MaterialInfoGPU>   g_GlobalMaterialInfo   : register(t3, space4);
StructuredBuffer<MaterialColor>     g_GlobalMaterials      : register(t4, space5);
Texture2D                           g_GBufferHeap[]        : register(t5, space6);
Texture2D                           g_LightMapHeap[]       : register(t6, space7);
SamplerState                        g_NearestRepeatSampler : register(s0);
SamplerState                        g_LinearRepeatSampler  : register(s1);
ConstantBuffer<RaytracingDataCB>    g_RaytracingData       : register(b0);
ConstantBuffer<CameraCB>            g_Camera               : register(b1);
ConstantBuffer<TonemapCB>           g_TonemapCB            : register(b2);

struct PixelShaderOutput
{
    float4 ColorTexture : SV_TARGET0;
};

PixelShaderOutput main(float2 TexCoord : TEXCOORD)
{
    PixelShaderOutput OUT;

    GFragment fi = UnpackGBuffer(g_GBufferHeap, g_NearestRepeatSampler, TexCoord);       

    // Extract lighting data
    float4 directDiffuse = g_LightMapHeap[LIGHTBUFFER_DIRECT_LIGHT].Sample(g_NearestRepeatSampler, TexCoord);
    float4 denoisedIndirectDiffuse = REBLUR_BackEnd_UnpackRadianceAndNormHitDist(
        g_LightMapHeap[LIGHTBUFFER_DENOISED_INDIRECT_DIFFUSE].Sample(g_NearestRepeatSampler, TexCoord));
    float4 denoisedIndirectSpecular = REBLUR_BackEnd_UnpackRadianceAndNormHitDist(
        g_LightMapHeap[LIGHTBUFFER_DENOISED_INDIRECT_SPECULAR].Sample(g_NearestRepeatSampler, TexCoord));
    float denoisedShadow = SIGMA_BackEnd_UnpackShadow(
        g_LightMapHeap[LIGHTBUFFER_DENOISED_SHADOW].Sample(g_NearestRepeatSampler, TexCoord));
    float4 transparentColor = g_LightMapHeap[LIGHTBUFFER_TRANSPARENT_COLOR].Sample(g_NearestRepeatSampler, TexCoord);
    
    float3 fragPos = GetWorldPosFromDepth(TexCoord, fi.depth, g_Camera.invViewProj);
    float3 V = -normalize(fragPos - g_Camera.pos);
    float3 N = fi.normal;
    float3 F0 = BaseColorToSpecularF0(fi.albedo, fi.metallic);
    
    // Calculations for diffuse and specular modulation
    float alpha = fi.roughness * fi.roughness;
    float NdotV = dot(N, V);
    float3 fenv = ApproxSpecularIntegralGGX(F0, alpha, NdotV);
    float3 diffDemod = (1.f - fenv) * BaseColorToDiffuseReflectance(fi.albedo, fi.metallic);
    float3 specDemod = fenv;

    denoisedIndirectDiffuse.rgb *= (diffDemod * 0.99f + 0.01f);
    denoisedIndirectSpecular.rgb *= (specDemod * 0.99f + 0.01f);

    // Combine lighting contributions
    float3 color = (directDiffuse.rgb * denoisedShadow) + denoisedIndirectDiffuse.rgb + denoisedIndirectSpecular.rgb + fi.emissive;
    color = lerp(color, transparentColor.rgb, transparentColor.a);
    
    // Debug options
    if (g_RaytracingData.debugSettings == DEBUG_LIGHTBUFFER_DENOISED_INDIRECT_DIFFUSE)
    {
        OUT.ColorTexture = float4(denoisedIndirectDiffuse.rgb / (diffDemod * 0.99f + 0.01f), 1.f);
    }
    else if (g_RaytracingData.debugSettings == DEBUG_LIGHTBUFFER_DENOISED_INDIRECT_SPECULAR)
    {
        OUT.ColorTexture = float4(denoisedIndirectSpecular.rgb / (specDemod * 0.99f + 0.01f), 1.f);
    }
    else
    {
        OUT.ColorTexture = float4(color, 1.f);
    }

    return OUT;
}