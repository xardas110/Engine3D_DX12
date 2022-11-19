#define HLSL
#include "../Common/TypesCompat.h"
#include "../Common/MaterialAttributes.hlsl"
#include "../Common/Math.hlsl"

ConstantBuffer<ObjectCB>            g_ObjectCB                  : register(b0);
Texture2D                           g_GlobalTextureData[]       : register(t1, space0);
StructuredBuffer<MaterialInfo>      g_GlobalMaterialInfo        : register(t3, space4);
StructuredBuffer<Material>          g_GlobalMaterials           : register(t4, space5);

SamplerState                        g_LinearRepeatSampler       : register(s0);

struct PixelShaderInput
{
	float4 PositionWS	:	POSITION;
	float3 NormalWS		:	NORMAL;
	float2 TexCoord		:	TEXCOORD;
    float3 Tangent_N    : TANGENT_N;
    float3 BiTangent_N  : BiTangent_N;
    float3 NormalLS_N   : NORMALLS_N;
};

struct PixelShaderOutput
{
    float4 albedo       : SV_TARGET0;
    float4 normalHeight : SV_TARGET1;
    float4 PBR          : SV_TARGET2;
    float4 emissive     : SV_TARGET3;
};

PixelShaderOutput main(PixelShaderInput IN)
{
    PixelShaderOutput OUT;
    
    MaterialInfo matInfo = g_GlobalMaterialInfo[g_ObjectCB.materialGPUID];   
    
    SurfaceMaterial surface = GetSurfaceMaterial(matInfo, IN.TexCoord, g_LinearRepeatSampler, g_GlobalTextureData);
    surface.normal = TangentToWorldNormal(IN.Tangent_N, IN.BiTangent_N, IN.NormalLS_N, surface.normal, g_ObjectCB.model);

    OUT.albedo = float4(surface.albedo, 1.f);
    OUT.normalHeight = float4(surface.normal, surface.height);
    OUT.PBR = float4(surface.ao, surface.roughness, surface.metallic, 1.f);
    OUT.emissive = float4(surface.emissive, 0.5f);
    
    if (matInfo.materialID != 0xffffffff)
    {
        Material mat;
        mat = g_GlobalMaterials[matInfo.materialID];
        OUT.albedo.rgb *= mat.color.rgb;
    }
    
    return OUT;
}
