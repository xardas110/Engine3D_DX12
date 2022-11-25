#define HLSL
#include "../Common/TypesCompat.h"
#include "../Common/MaterialAttributes.hlsl"
#include "../Common/Math.hlsl"
#include "GBuffer.hlsl"

ConstantBuffer<ObjectCB>            g_ObjectCB                  : register(b0);
ConstantBuffer<CameraCB>            g_CameraCB                  : register(b1);
    
Texture2D                           g_GlobalTextureData[]       : register(t1, space0);
StructuredBuffer<MaterialInfo>      g_GlobalMaterialInfo        : register(t3, space4);
StructuredBuffer<Material>          g_GlobalMaterials           : register(t4, space5);
  
SamplerState                        g_LinearRepeatSampler       : register(s0);

struct PixelShaderInput
{
	float4 PositionWS	:	POSITION;
	float3 NormalWS		:	NORMAL;
	float2 TexCoord		:	TEXCOORD;
    float3 Tangent_N    :   TANGENT_N;
    float3 BiTangent_N  :   BiTangent_N;
    float3 NormalLS_N   :   NORMALLS_N;
    float4 Position		:   SV_Position;
};

    
 //Make sure that GBUFFER_XXX defines match the rendertarget   
struct PixelShaderOutput
{
    float4 albedo           : SV_TARGET0;
    float4 normalRoughness  : SV_TARGET1;
    float4 motionVector     : SV_TARGET2;
    float4 emissiveSM       : SV_TARGET3;
    float4 aoMetallicHeight : SV_TARGET4;
    float linearDepth       : SV_TARGET5;     
};

PixelShaderOutput main(PixelShaderInput IN)
{
    PixelShaderOutput OUT;
    
    MaterialInfo matInfo = g_GlobalMaterialInfo[g_ObjectCB.materialGPUID];   
           
    bool bMatHasNormal;
    SurfaceMaterial surface = GetSurfaceMaterial(matInfo, IN.TexCoord, bMatHasNormal, g_LinearRepeatSampler, g_GlobalTextureData);
        
    if (bMatHasNormal == true)
    {    
        surface.normal = TangentToWorldNormal(IN.Tangent_N, IN.BiTangent_N, IN.NormalLS_N, surface.normal, g_ObjectCB.model);
    }
    else
    {
        surface.normal = IN.NormalWS;
    }

    GPackInfo gPack = PackGBuffer(g_CameraCB, g_ObjectCB, surface, IN.PositionWS.rgb, IN.Position.z);
    
    OUT.albedo = gPack.albedo;
    OUT.aoMetallicHeight = gPack.aoMetallicHeight;
    OUT.emissiveSM = gPack.emissiveSM;
    OUT.linearDepth = gPack.linearDepth;
    OUT.motionVector = gPack.motionVector;
    OUT.normalRoughness = gPack.normalRough;
        
    return OUT;
}
