#ifndef MATH_H
#define MATH_H

// Rotate a point by a quaternion
float3 RotatePoint(float4 q, float3 v)
{
    const float3 qAxis = float3(q.x, q.y, q.z);
    return 2.0f * dot(qAxis, v) * qAxis + (q.w * q.w - dot(qAxis, qAxis)) * v + 2.0f * q.w * cross(qAxis, v);
}

// Construct a TBN matrix from model, tangent, bitangent, and local normal
float3x3 ConstructTBN(in float3x3 model, in float3 tangent, in float3 biTangent, in float3 localNormal)
{
    float3 T = normalize(float3(mul(model, tangent)));
    float3 B = normalize(float3(mul(model, biTangent)));
    float3 N = normalize(float3(mul(model, localNormal)));

    return transpose(float3x3(T, B, N));
}

// Convert tangent space normal to world space normal using TBN matrix
float3 GetWorldNormal(in float3 tangentNormal, in float3x3 TBN)
{
    float3 normal = tangentNormal * 2.f - 1.f;
    normal = normalize(mul(TBN, normal));
    return normal;
}

// Convert UV coordinates and depth to world space position
float3 GetWorldPosFromDepth(in float2 uv, float depth, in float4x4 invViewProj)
{
    float4 ndc = float4(uv * 2.0f - 1.0f, depth, 1.0f);
    ndc.y *= -1.0f;
    float4 wp = mul(invViewProj, ndc);
    return (wp / wp.w).xyz;
}

// Convert UV coordinates to clip space
float2 UVToClip(float2 UV)
{
    return float2(UV.x * 2 - 1, 1 - UV.y * 2);
}

// Convert clip space coordinates to UV coordinates
float2 ClipToUV(float2 ClipPos)
{
    return float2(ClipPos.x * 0.5 + 0.5, 0.5 - ClipPos.y * 0.5);
}

// Pi function for a single float value
float Pi(float x)
{
    return radians(180.0 * x);
}

// Pi function for float2, float3, and float4 values
float2 Pi(float2 x)
{
    return radians(180.0 * x);
}

float3 Pi(float3 x)
{
    return radians(180.0 * x);
}

float4 Pi(float4 x)
{
    return radians(180.0 * x);
}

// Square root for values in the range [0, 1]
float Sqrt01(float x)
{
    return sqrt(saturate(x));
}

float2 Sqrt01(float2 x)
{
    return sqrt(saturate(x));
}

float3 Sqrt01(float3 x)
{
    return sqrt(saturate(x));
}

float4 Sqrt01(float4 x)
{
    return sqrt(saturate(x));
}

// Power function for values in the range [0, 1]
float Pow01(float x, float y)
{
    return pow(saturate(x), y);
}

float2 Pow01(float2 x, float y)
{
    return pow(saturate(x), y);
}

float2 Pow01(float2 x, float2 y)
{
    return pow(saturate(x), y);
}

float3 Pow01(float3 x, float y)
{
    return pow(saturate(x), y);
}

float3 Pow01(float3 x, float3 y)
{
    return pow(saturate(x), y);
}

float4 Pow01(float4 x, float y)
{
    return pow(saturate(x), y);
}

float4 Pow01(float4 x, float4 y)
{
    return pow(saturate(x), y);
}

// Convert linear space color to sRGB space
float3 LinearToSrgb(float3 color)
{
    const float4 consts = float4(1.055, 0.41666, -0.055, 12.92);
    color = saturate(color);

    return lerp(consts.x * pow(color, consts.yyy) + consts.zzz, consts.w * color, color < 0.0031308);
}

// Convert sRGB space color to linear space
float3 SrgbToLinear(float3 srgbColor)
{
    const float4 consts = float4(1.0 / 12.92, 1.0 / 1.055, 0.055 / 1.055, 2.4);
    srgbColor = saturate(srgbColor);
    return lerp(srgbColor * consts.x, pow(srgbColor * consts.y + consts.zzz, consts.www), srgbColor > 0.04045);
}
#endif