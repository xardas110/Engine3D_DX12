#ifndef GEOMETRYPASS_HLSL
#define GEOMETRYPASS_HLSL

Texture2D g_AlbedoTexture			:	register(t0);
Texture2D g_NormalTexture			:	register(t1);
Texture2D g_AOTexture				:	register(t2);
Texture2D g_RoughnessTexture		:	register(t3);
Texture2D g_MetallicTexture			:	register(t4);
Texture2D g_OpacityTexture			:	register(t5);

SamplerState g_LinearRepeatSampler	:	register(s0);

struct PixelShaderInput
{
	float4 PositionWS : POSITION;
	float3 NormalWS : NORMAL;
	float2 TexCoord : TEXCOORD;
};

float4 main(PixelShaderInput IN) : SV_Target
{
    return g_AlbedoTexture.Sample(g_LinearRepeatSampler, IN.TexCoord);
}

#endif