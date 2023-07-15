#define HLSL
#include "../Common/TypesCompat.h"
#include "../Common/MaterialAttributes.hlsl"
#include "../Common/Math.hlsl"
#include "GBuffer.hlsl"

ConstantBuffer<ObjectCB>            g_ObjectCB                  : register(b0);
ConstantBuffer<CameraCB>            g_CameraCB                  : register(b1);
    
Texture2D                           g_GlobalTextureData[]       : register(t1, space0);
StructuredBuffer<MaterialInfoGPU>   g_GlobalMaterialInfo : register(t3, space4);
StructuredBuffer<Material>          g_GlobalMaterials           : register(t4, space5);
  
SamplerState                        g_LinearRepeatSampler       : register(s0);

struct PixelShaderInput
{
	float4 PositionWS	:	POSITION;
    float4 PrevPositionWS : PREVPOS;
	float3 NormalWS		:	NORMAL;
	float2 TexCoord		:	TEXCOORD;
    float3 Tangent_N    :   TANGENT_N;
    float3 BiTangent_N  :   BiTangent_N;
    float3 NormalLS_N   :   NORMALLS_N;
    float4 PrevPosition :   PREVPOS_CLIP;
    float4 PositionClip :   POS_CLIP;
    float  ViewZ        :   VIEW_Z;
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
    float4 geometryNormal   : SV_TARGET6;
    float2 motion2D         : SV_TARGET7;
};

static float alphaCutoff = 0.5f;

PixelShaderOutput main(PixelShaderInput IN)
{
    PixelShaderOutput OUT;
    
    MaterialInfoGPU matInfo = g_GlobalMaterialInfo[g_ObjectCB.materialGPUID];
       
    if (matInfo.flags & INSTANCE_ALPHA_BLEND)
        discard;
    
    MeshVertex vert;
    vert.textureCoordinate = IN.TexCoord;
    vert.normal = IN.NormalLS_N;
    vert.tangent = IN.Tangent_N;
    vert.bitangent = IN.BiTangent_N;
    vert.position = IN.PositionClip;

    SurfaceMaterial surface = GetSurfaceMaterial(matInfo, vert, g_ObjectCB.model, g_ObjectCB.objRotQuat, g_LinearRepeatSampler, g_GlobalTextureData, 0.f, SAMPLE_TYPE_MIP);
    ApplyMaterial(matInfo, surface, g_GlobalMaterials);
        
    if (surface.opacity < alphaCutoff)
        discard;
    
    GPackInfo gPack = PackGBuffer(g_CameraCB, g_ObjectCB, surface, IN.PositionWS.rgb, IN.PositionClip.z, IN.NormalWS);
      
    OUT.albedo = gPack.albedo;
    OUT.aoMetallicHeight = gPack.aoMetallicHeight;
    OUT.emissiveSM = gPack.emissiveSM;
    OUT.linearDepth = IN.PositionClip.z;
    OUT.normalRoughness = gPack.normalRough;
    OUT.geometryNormal = gPack.geometryNormal;
    OUT.motionVector = float4(IN.PrevPositionWS.rgb - IN.PositionWS.rgb, 1.f);
       
    float2 res = float2(g_CameraCB.resolution.x, g_CameraCB.resolution.y);
    
    float2 uv1 = ClipToUV(IN.PrevPosition.xy / IN.PrevPosition.w);
    float2 uv2 = ClipToUV(IN.PositionClip.xy / IN.PositionClip.w);

    OUT.motion2D = (uv1 * res) - (uv2 * res);
        
    return OUT;
}
