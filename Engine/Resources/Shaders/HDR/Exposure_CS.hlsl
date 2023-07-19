#define HLSL
#include "../Common/TypesCompat.h"
#include "../Common/BRDF.hlsl"

// Buffers and Constants
RWBuffer<uint>                  t_Histogram     : register(u0);
RWTexture2D<float>              u_Exposure      : register(u1);
ConstantBuffer<TonemapCB>       g_ToneMapping   : register(b2);

// Fetches a histogram bucket value, normalizing it
float GetHistogramBucket(uint BucketIndex)
{
    return float(t_Histogram[BucketIndex]) / FIXED_POINT_FRAC_MULTIPLIER;
}

// Computes the sum of the histogram bins
float ComputeHistogramSum()
{
    float sum = 0.0f;
    [loop]
    for (uint i = 0; i < HISTOGRAM_BINS; ++i)
    {
        sum += GetHistogramBucket(i);
    }
    return sum;
}

// Computes luminance from a given histogram position
float ComputeLuminanceFromHistogramPosition(float HistogramPosition)
{
    return exp2(HistogramPosition * g_ToneMapping.logLuminanceScale + g_ToneMapping.logLuminanceBias);
}

// Computes average luminance excluding outliers
float ComputeAverageLuminaneWithoutOutlier(float minFractionSum, float maxFractionSum)
{
    float2 sumWithoutOutliers = 0;

    [loop]
    for (uint i = 0; i < HISTOGRAM_BINS; ++i)
    {
        float localValue = GetHistogramBucket(i);

        // Remove lower end outlier
        float subtractionValue = min(localValue, minFractionSum);
        localValue -= subtractionValue;
        minFractionSum -= subtractionValue;
        maxFractionSum -= subtractionValue;

        // Remove upper end outlier
        localValue = min(localValue, maxFractionSum);
        maxFractionSum -= localValue;

        float luminanceAtBucket = ComputeLuminanceFromHistogramPosition(i / float(HISTOGRAM_BINS));

        sumWithoutOutliers += float2(luminanceAtBucket, 1) * localValue;
    }

    return sumWithoutOutliers.x / max(0.0001f, sumWithoutOutliers.y);
}

// Computes the target exposure based on the current histogram data
float ComputeEyeAdaptationExposure()
{
    float histogramSum = ComputeHistogramSum();

    float unclampedAdaptedLuminance = ComputeAverageLuminaneWithoutOutlier(
        histogramSum * g_ToneMapping.histogramLowPercentile,
        histogramSum * g_ToneMapping.histogramHighPercentile);

    return clamp(unclampedAdaptedLuminance, g_ToneMapping.minAdaptedLuminance, g_ToneMapping.maxAdaptedLuminance);
}

// Computes the adaptation factor between the old and target exposure
float ComputeEyeAdaptation(float oldExposure, float targetExposure, float frameTime)
{
    float diff = oldExposure - targetExposure;
    float adaptationSpeed = (diff < 0) ? g_ToneMapping.eyeAdaptationSpeedUp : g_ToneMapping.eyeAdaptationSpeedDown;

    // If the adaptation speed is zero or negative, immediately return the target exposure
    if (adaptationSpeed <= 0.0f)
        return targetExposure;

    float factor = exp2(-frameTime * adaptationSpeed);
    return targetExposure + diff * factor;
}

// Main compute shader to update the exposure texture
[numthreads(1, 1, 1)]
void main()
{
    float targetExposure = ComputeEyeAdaptationExposure();
    float oldExposure = u_Exposure[uint2(0, 0)];
    float smoothedExposure = ComputeEyeAdaptation(oldExposure, targetExposure, g_ToneMapping.frameTime);
    u_Exposure[uint2(0, 0)] = smoothedExposure;
}