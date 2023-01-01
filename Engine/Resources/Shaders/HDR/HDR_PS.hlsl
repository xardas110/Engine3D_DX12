#define HLSL
#include "../Common/TypesCompat.h"

#include "../Common/HDR.hlsl"
#include "../Common/MaterialAttributes.hlsl"

Texture2D                       g_Texture                   : register(t0);

RWTexture2D<float>              g_Exposure                  : register(u0);

ConstantBuffer<TonemapCB>       g_TonemapCB                 : register(b0);

SamplerState                    g_NearestRepeatSampler      : register(s0);
SamplerState                    g_LinearRepeatSampler       : register(s1);

struct PixelShaderOutput
{
    float4 ColorTexture : SV_TARGET0;
};

PixelShaderOutput main(float2 TexCoord : TEXCOORD)
{
    PixelShaderOutput OUT;
        
    float3 color = g_Texture.Sample(g_LinearRepeatSampler, TexCoord).rgb;
    
    TonemapCB tonemap = g_TonemapCB;
    tonemap.Exposure = g_Exposure[uint2(0, 0)];
    
    color = ApplyTonemap(color, tonemap);

    OUT.ColorTexture = float4(color, 1.f);
        
    return OUT;
}