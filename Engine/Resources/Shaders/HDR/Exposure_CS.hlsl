#define HLSL
#include "../Common/TypesCompat.h"
#include "../Common/BRDF.hlsl"

Buffer<uint> t_Histogram : register(t0);
RWTexture2D<float> u_Exposure : register(u0);
ConstantBuffer<TonemapCB> g_ToneMapping : register(b2);


// Most of the code below has been shamelessly stolen from UE4 eye adaptation shaders, i.e. PostProcessEyeAdaptation.usf

float GetHistogramBucket(uint BucketIndex)
{
    return float(t_Histogram[BucketIndex]) / FIXED_POINT_FRAC_MULTIPLIER;
}

float ComputeHistogramSum()
{
    float Sum = 0;

    [loop]
    for (uint i = 0; i < HISTOGRAM_BINS; ++i)
    {
        Sum += GetHistogramBucket(i);
    }

    return Sum;
}

float ComputeLuminanceFromHistogramPosition(float HistogramPosition)
{
    return exp2(HistogramPosition * g_ToneMapping.logLuminanceScale + g_ToneMapping.logLuminanceBias);
}

[numthreads(1, 1, 1)]
void main()
{
    u_Exposure[uint2(0, 0)] = 0.1f;
}
