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
    float3x3 TBN        :   TBN;
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
    
    float4 albedo = GetAlbedo(matInfo, IN.TexCoord, g_LinearRepeatSampler, g_GlobalTextureData);
    float3 normal = GetNormal(matInfo, IN.TexCoord, g_LinearRepeatSampler, g_GlobalTextureData);
    normal = GetWorldNormal(normal, IN.TBN);
    normal *= 0.5f + 0.5f;
    
    float ao = GetAO(matInfo, IN.TexCoord, g_LinearRepeatSampler, g_GlobalTextureData);
    float roughness = GetRoughness(matInfo, IN.TexCoord, g_LinearRepeatSampler, g_GlobalTextureData);
    float metallic = GetMetallic(matInfo, IN.TexCoord, g_LinearRepeatSampler, g_GlobalTextureData);
    float height = GetHeight(matInfo, IN.TexCoord, g_LinearRepeatSampler, g_GlobalTextureData);
    
    float3 emissive = GetEmissive(matInfo, IN.TexCoord, g_LinearRepeatSampler, g_GlobalTextureData);
    
    float opacity = GetOpacity(matInfo, IN.TexCoord, g_LinearRepeatSampler, g_GlobalTextureData);
       
    OUT.albedo = albedo;
    OUT.normalHeight = float4(normal, height);
    OUT.PBR = float4(ao, roughness, metallic, 1.f);
    OUT.emissive = float4(emissive, 1.f);
    
    if (matInfo.materialID != 0xffffffff)
    {
        Material mat;
        mat = g_GlobalMaterials[matInfo.materialID];
        OUT.albedo.rgb *= mat.color.rgb;
    }
    
    return OUT;
}
