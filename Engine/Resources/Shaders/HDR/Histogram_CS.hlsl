#define HLSL
#include "../Common/TypesCompat.h"
#include "../Common/BRDF.hlsl"

Texture2D                   t_Source        : register(t0);
RWBuffer<uint>              u_Histogram     : register(u0);
ConstantBuffer<TonemapCB>   g_ToneMapping   : register(b2);

groupshared uint s_Histogram[HISTOGRAM_BINS];

[numthreads(HISTROGRAM_GROUP_X, HISTROGRAM_GROUP_Y, 1)]
void main(uint linearIdx : SV_GroupIndex, uint2 globalIdx : SV_DispatchThreadID)
{
    uint2 pixelPos = globalIdx.xy + g_ToneMapping.viewOrigin.xy;
    bool valid = all(globalIdx.xy < g_ToneMapping.viewSize.xy);

    if (linearIdx < HISTOGRAM_BINS)
    {
        s_Histogram[linearIdx] = 0;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    if (valid)
    {
        float3 color = t_Source[pixelPos].rgb;

        float luminance = Luminance(color);
        float biasedLogLuminance = log2(luminance) * g_ToneMapping.logLuminanceScale + g_ToneMapping.logLuminanceBias;
        float histogramBin = saturate(biasedLogLuminance) * (HISTOGRAM_BINS - 1);
        
        uint leftBin = uint(floor(histogramBin));
        uint rightBin = leftBin + 1;

        uint rightWeight = uint(frac(histogramBin) * FIXED_POINT_FRAC_MULTIPLIER);
        uint leftWeight = FIXED_POINT_FRAC_MULTIPLIER - rightWeight;

        if (leftWeight != 0 && leftBin < HISTOGRAM_BINS) 
            InterlockedAdd(s_Histogram[leftBin], leftWeight);
        if (rightWeight != 0 && rightBin < HISTOGRAM_BINS) 
            InterlockedAdd(s_Histogram[rightBin], rightWeight);
    }

    GroupMemoryBarrierWithGroupSync();

    if (linearIdx < HISTOGRAM_BINS)
    {
        uint localBinValue = s_Histogram[linearIdx];
        if (localBinValue != 0)
            InterlockedAdd(u_Histogram[linearIdx], localBinValue);
    }
}
