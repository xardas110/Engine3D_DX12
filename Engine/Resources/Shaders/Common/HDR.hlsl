#ifndef HDR_HLSL
#define HDR_HLSL

#define hlsl
#include "../Common/TypesCompat.h"

// Linear Tonemapping
// Just normalizing based on some maximum luminance
float3 Linear(float3 HDR, float max)
{
    float3 SDR = HDR;
    if (max > 0)
    {
        SDR = saturate(HDR / max);
    }

    return SDR;
}

// Reinhard tone mapping.
// See: http://www.cs.utah.edu/~reinhard/cdrom/tonemap.pdf
float3 Reinhard(float3 HDR, float k)
{
    return HDR / (HDR + k);
}

// Reinhard^2
// See: http://www.cs.utah.edu/~reinhard/cdrom/tonemap.pdf
float3 ReinhardSqr(float3 HDR, float k)
{
    return pow(Reinhard(HDR, k), 2);
}

// ACES Filmic
// See: https://www.slideshare.net/ozlael/hable-john-uncharted2-hdr-lighting/142
float3 ACESFilmic(float3 x, float A, float B, float C, float D, float E, float F)
{
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - (E / F);
}

float Tonemap_Unreal(float x)
{
    // Unreal 3, Documentation: "Color Grading"
    // Adapted to be close to Tonemap_ACES, with similar range
    // Gamma 2.2 correction is baked in, don't use with sRGB conversion!
    return x / (x + 0.155) * 1.019;
}

float3 Uncharted2ToneMapping(in float3 color, in TonemapCB p)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    float exposure = 2.;
    color *= exposure;
    color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
    float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
    color /= white;
    return color;
}

float3 ApplyTonemap(in float3 hdr, in TonemapCB tonemapParamaters)
{
    // Add exposure to HDR result.
    hdr *= exp2(tonemapParamaters.Exposure);

    // Perform tonemapping on HDR image.
    float3 SDR = (float3) 0;
    switch (tonemapParamaters.TonemapMethod)
    {
        case TM_Linear:
            SDR = Linear(hdr, tonemapParamaters.MaxLuminance);
            break;
        case TM_Reinhard:
            SDR = Reinhard(hdr, tonemapParamaters.K);
            break;
        case TM_ReinhardSq:
            SDR = ReinhardSqr(hdr, tonemapParamaters.K);
            break;
        case TM_ACESFilmic:
            SDR = ACESFilmic(hdr, tonemapParamaters.A, tonemapParamaters.B, tonemapParamaters.C, tonemapParamaters.D, tonemapParamaters.E, tonemapParamaters.F) /
              ACESFilmic(tonemapParamaters.LinearWhite, tonemapParamaters.A, tonemapParamaters.B, tonemapParamaters.C, tonemapParamaters.D, tonemapParamaters.E, tonemapParamaters.F);
            break;
        case TM_Uncharted:
            SDR = Uncharted2ToneMapping(hdr, tonemapParamaters);
            break;
    }

    return pow(abs(SDR), 1.0f / tonemapParamaters.Gamma);

}

#endif