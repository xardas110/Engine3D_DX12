#define HLSL
#include "../Common/TypesCompat.h"

#include "../Common/HDR.hlsl"
#include "../Common/MaterialAttributes.hlsl"

Texture2D                       g_Texture                   : register(t0);
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
    color = ApplyTonemap(color, g_TonemapCB);

    OUT.ColorTexture = float4(color, 1.f);
        
    return OUT;
}