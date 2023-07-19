#ifndef HDR_HLSL
#define HDR_HLSL

#define hlsl
#include "../Common/TypesCompat.h"
#include "../Common/MaterialAttributes.hlsl"

// Linear Tonemapping
// Normalize HDR values based on a maximum luminance
float3 Linear(float3 HDR, float max)
{
    float3 SDR = HDR;
    if (max > 0)
    {
        SDR = saturate(HDR / max);
    }

    return SDR;
}

// Reinhard tone mapping
// See: http://www.cs.utah.edu/~reinhard/cdrom/tonemap.pdf
float3 Reinhard(float3 HDR, float k)
{
    return HDR / (HDR + k);
}

// Reinhard^2 tone mapping
// See: http://www.cs.utah.edu/~reinhard/cdrom/tonemap.pdf
float3 ReinhardSqr(float3 HDR, float k)
{
    return pow(Reinhard(HDR, k), 2);
}

// ACES Filmic tone mapping
// See: https://www.slideshare.net/ozlael/hable-john-uncharted2-hdr-lighting/142
float3 ACESFilmic(float3 x, float A, float B, float C, float D, float E, float F)
{
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - (E / F);
}

float3 _UnchartedCurve(float3 color)
{
    float A = 0.22; // Shoulder Strength
    float B = 0.3; // Linear Strength
    float C = 0.1; // Linear Angle
    float D = 0.2; // Toe Strength
    float E = 0.01; // Toe Numerator
    float F = 0.3; // Toe Denominator

    return saturate(((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - (E / F));
}

float3 HdrToLinear_Uncharted(float3 color)
{
    // John Hable's Uncharted 2 filmic tone map (http://filmicgames.com/archives/75)
    return saturate(_UnchartedCurve(color) / _UnchartedCurve(11.2).x);
}

float3 ApplyTonemap(in float3 hdr, in TonemapCB tonemapParameters)
{
    // Add exposure to HDR result
    hdr *= exp2(tonemapParameters.Exposure);

    // Perform tonemapping on HDR image
    float3 SDR = float3(0, 0, 0);
    switch (tonemapParameters.TonemapMethod)
    {
        case TM_Linear:
            SDR = Linear(hdr, tonemapParameters.MaxLuminance);
            break;
        case TM_Reinhard:
            SDR = Reinhard(hdr, tonemapParameters.K);
            break;
        case TM_ReinhardSq:
            SDR = ReinhardSqr(hdr, tonemapParameters.K);
            break;
        case TM_ACESFilmic:
            SDR = ACESFilmic(hdr, tonemapParameters.A, tonemapParameters.B, tonemapParameters.C, tonemapParameters.D, tonemapParameters.E, tonemapParameters.F) /
                  ACESFilmic(tonemapParameters.LinearWhite, tonemapParameters.A, tonemapParameters.B, tonemapParameters.C, tonemapParameters.D, tonemapParameters.E, tonemapParameters.F);
            break;
        case TM_Uncharted:
            SDR = HdrToLinear_Uncharted(hdr);
            break;
    }

    return LinearToSrgb(SDR);
}

#endif