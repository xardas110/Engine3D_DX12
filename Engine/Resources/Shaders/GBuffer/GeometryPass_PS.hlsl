#define HLSL
#include "../Common/TypesCompat.h"

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
};

float4 GetAlbedo(in MaterialInfo matInfo, in float2 texCoords)
{
    if (matInfo.albedo != 0xffffffff)
    {
        Texture2D albedo = g_GlobalTextureData[matInfo.albedo];
        return albedo.Sample(g_LinearRepeatSampler, texCoords);
    }
    return float4(1.f, 0.f, 0.f, 1.f);
}

void ApplyMaterialProperties(in Material mat, inout float4 albedo, inout float3 emissive, in float3 transparency)
{
    albedo *= mat.color;
    emissive *= mat.emissive;
    transparency *= mat.transparent;
}

float4 main(PixelShaderInput IN) : SV_Target
{
    MaterialInfo matInfo = g_GlobalMaterialInfo[g_ObjectCB.materialGPUID];   
    float4 albedo = GetAlbedo(matInfo, IN.TexCoord);
    
    return albedo;
}
