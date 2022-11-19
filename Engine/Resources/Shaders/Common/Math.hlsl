#ifndef MATH_H
#define MATH_H

float3x3 ConstructTBN(
    in float3x3 model, 
    in float3 tangent, 
    in float3 biTangent, 
    in float3 localNormal)
{
    float3 T = normalize(float3(mul(model, tangent)));
    float3 B = normalize(float3(mul(model, biTangent)));
    float3 N = normalize(float3(mul(model, localNormal)));

    return transpose(float3x3(T, B, N));
}

//Tangent normal in texturecoords
float3 GetWorldNormal(in float3 tangentNormal, in float3x3 TBN)
{
    float3 normal;
    normal = tangentNormal * 2.f - 1.f;
    normal = normalize(mul(TBN, normal));
    return normal;
}

float3 GetWorldPosFromDepth(in float2 uv, float depth, in float4x4 invViewProj)
{
    float4 ndc = float4(uv * 2.0f - 1.0f, depth, 1.0f);
    ndc.y *= -1.0f;
    float4 wp = mul(ndc, invViewProj);
    return (wp / wp.w).xyz;
}

#endif