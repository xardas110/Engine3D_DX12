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
    // Calculate the global pixel position taking into account the view origin.
    uint2 pixelPos = globalIdx.xy + g_ToneMapping.viewOrigin.xy;

    // Check if the current thread is within the valid viewport size.
    bool valid = all(globalIdx.xy < g_ToneMapping.viewSize.xy);

    // Initialize shared histogram bins to zero for all threads.
    if (linearIdx < HISTOGRAM_BINS)
    {
        s_Histogram[linearIdx] = 0;
    }
    
    // Ensure all threads have finished initializing before proceeding.
    GroupMemoryBarrierWithGroupSync();
    
    if (valid)
    {
        // Sample the source texture.
        float3 color = t_Source[pixelPos].rgb;

        // Calculate luminance and find its position in the histogram.
        float luminance = Luminance(color);
        float biasedLogLuminance = log2(luminance) * g_ToneMapping.logLuminanceScale + g_ToneMapping.logLuminanceBias;
        float histogramBin = saturate(biasedLogLuminance) * (HISTOGRAM_BINS - 1);

        // Determine which bins the luminance will be distributed between.
        uint leftBin = uint(floor(histogramBin));
        uint rightBin = leftBin + 1;

        // Calculate weights for left and right bins based on fractional part of histogramBin.
        uint rightWeight = uint(frac(histogramBin) * FIXED_POINT_FRAC_MULTIPLIER);
        uint leftWeight = FIXED_POINT_FRAC_MULTIPLIER - rightWeight;

        // Update the shared histogram with the calculated weights.
        if (leftWeight != 0 && leftBin < HISTOGRAM_BINS) 
            InterlockedAdd(s_Histogram[leftBin], leftWeight);
        if (rightWeight != 0 && rightBin < HISTOGRAM_BINS) 
            InterlockedAdd(s_Histogram[rightBin], rightWeight);
    }

    // Synchronize to ensure all threads have finished updating the shared histogram.
    GroupMemoryBarrierWithGroupSync();

    // Commit the values from the shared histogram to the global histogram.
    if (linearIdx < HISTOGRAM_BINS)
    {
        uint localBinValue = s_Histogram[linearIdx];
        if (localBinValue != 0)
            InterlockedAdd(u_Histogram[linearIdx], localBinValue);
    }
}
