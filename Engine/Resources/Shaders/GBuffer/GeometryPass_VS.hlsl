
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
	float3 NormalWS		: NORMAL;
	float2 TexCoord		: TEXCOORD;
    float3x3 TBN		: TBN;
	float4 Position		: SV_Position;
};

VertexShaderOutput main(VertexPositionNormalTexture IN)
{  
	VertexShaderOutput OUT;
	OUT.Position = mul(g_ObjectCB.mvp, float4(IN.Position, 1.0f));
	OUT.PositionWS = mul(g_ObjectCB.model, float4(IN.Position, 1.f));
	OUT.NormalWS = mul((float3x3)g_ObjectCB.transposeInverseModel, IN.Normal);
	OUT.NormalWS = normalize(OUT.NormalWS);
	OUT.TexCoord = IN.TexCoord; 
    OUT.TBN = ConstructTBN(g_ObjectCB.model, IN.Tangent, IN.BiTangent, normalize(IN.Normal));
	return OUT;
}
