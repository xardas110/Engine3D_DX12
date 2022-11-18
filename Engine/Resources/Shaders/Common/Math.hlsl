#pragma once

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