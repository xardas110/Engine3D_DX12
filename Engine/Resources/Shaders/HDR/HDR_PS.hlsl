#define HLSL
#include "../Common/TypesCompat.h"

#include "../Common/HDR.hlsl"
#include "../Common/BRDF.hlsl"
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

float3 ConvertToLDR(float3 color)
{
    float srcLuminance = Luminance(color);

    if (srcLuminance <= 0)
        return 0;

    float adaptedLuminance = g_Exposure[uint2(0, 0)];
    if (adaptedLuminance <= 0)
        adaptedLuminance = g_TonemapCB.minAdaptedLuminance;

    float scaledLuminance = g_TonemapCB.exposureScale * srcLuminance / adaptedLuminance;
    float mappedLuminance = (scaledLuminance * (1 + scaledLuminance * g_TonemapCB.whitePointInvSquared)) / (1 + scaledLuminance);

    return color * (mappedLuminance / srcLuminance);
}

PixelShaderOutput main(float2 TexCoord : TEXCOORD)
{
    PixelShaderOutput OUT;
        
    float3 color = g_Texture.Sample(g_LinearRepeatSampler, TexCoord).rgb;
    
    if (g_TonemapCB.bEyeAdaption == 1)
        color = ConvertToLDR(color);
    else
        color = ApplyTonemap(color, g_TonemapCB);

    OUT.ColorTexture = float4(color, 1.f);
        
    return OUT;
}