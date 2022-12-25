
#define HLSL
#include "../Common/TypesCompat.h"
#include "../Common/Math.hlsl"

ConstantBuffer<ObjectCB> g_ObjectCB : register(b0);

struct VertexPositionNormalTexture
{
	float3 Position			: POSITION;
	float3 Normal			: NORMAL;
	float2 TexCoord			: TEXCOORD;
    float3 Tangent			: TANGENT;
    float3 BiTangent		: BITANGENT;
};

struct VertexShaderOutput
{
	float4 PositionWS	: POSITION;
    float4 PrevPositionWS : PREVPOS;
	float3 NormalWS		: NORMAL;
	float2 TexCoord		: TEXCOORD;
    float3 Tangent_N	: TANGENT_N;
    float3 BiTangent_N	: BiTangent_N;
    float3 NormalLS_N	: NORMALLS_N;
    float4 PrevPosition : PREVPOS_CLIP;
    float4 PositionClip : POS_CLIP;
    float	ViewZ		: VIEW_Z;
	float4 Position		: SV_Position;
};

VertexShaderOutput main(VertexPositionNormalTexture IN)
{  
	VertexShaderOutput OUT;
	OUT.Position = mul(g_ObjectCB.mvp, float4(IN.Position, 1.0f));
    OUT.PositionClip = OUT.Position;
    
    OUT.PrevPosition = mul(g_ObjectCB.prevMVP, float4(IN.Position, 1.f));
		
	OUT.PositionWS = mul(g_ObjectCB.model, float4(IN.Position, 1.f));
    OUT.PrevPositionWS = mul(g_ObjectCB.prevModel, float4(IN.Position, 1.f));
		
	
    OUT.ViewZ = mul(g_ObjectCB.view, float4(OUT.PositionWS.xyz, 1.0f)).z;
	
	
	OUT.NormalWS = mul((float3x3)g_ObjectCB.transposeInverseModel, IN.Normal);
	OUT.NormalWS = normalize(OUT.NormalWS);
		
	OUT.TexCoord = IN.TexCoord;
		
    OUT.Tangent_N = IN.Tangent;
    OUT.BiTangent_N = IN.BiTangent;
    OUT.NormalLS_N = IN.Normal;
	
	return OUT;
}
