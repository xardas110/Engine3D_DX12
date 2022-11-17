#define hlsl 
#include "TypesCompat.h"

#ifndef MATERIAL_ATTRIBUTES_HLSL
#define MATERIAL_ATTRIBUTES_HLSL

float4 GetAlbedo(in MaterialInfo matInfo, in float2 texCoords, in SamplerState inSampler, in Texture2D globalTextureData[])
{
    if (matInfo.albedo != 0xffffffff)
    {
        Texture2D albedo = globalTextureData[matInfo.albedo];
        return albedo.Sample(inSampler, texCoords);
    }
    return float4(1.f, 0.f, 0.f, 1.f);
}

float3 GetNormal(in MaterialInfo matInfo, in float2 texCoords, in SamplerState inSampler, in Texture2D globalTextureData[])
{
    if (matInfo.normal != 0xffffffff)
    {
        Texture2D normal = globalTextureData[matInfo.normal];
        return normal.Sample(inSampler, texCoords).rgb;
    }
    return float3(0.f, 1.f, 0.f);
}

float GetAO(in MaterialInfo matInfo, in float2 texCoords, in SamplerState inSampler, in Texture2D globalTextureData[])
{
    if (matInfo.ao != 0xffffffff)
    {
        Texture2D ao = globalTextureData[matInfo.ao];
        return ao.Sample(inSampler, texCoords).r;
    }
    return 1.f;
}

float GetRoughness(in MaterialInfo matInfo, in float2 texCoords, in SamplerState inSampler, in Texture2D globalTextureData[])
{
    if (matInfo.roughness != 0xffffffff)
    {
        Texture2D roughness = globalTextureData[matInfo.roughness];
        return roughness.Sample(inSampler, texCoords).r;
    }
    return 0.f;
}

float GetMetallic(in MaterialInfo matInfo, in float2 texCoords, in SamplerState inSampler, in Texture2D globalTextureData[])
{
    if (matInfo.metallic != 0xffffffff)
    {
        Texture2D metallic = globalTextureData[matInfo.metallic];
        return metallic.Sample(inSampler, texCoords).r;
    }
    return 0.f;
}

float GetHeight(in MaterialInfo matInfo, in float2 texCoords, in SamplerState inSampler, in Texture2D globalTextureData[])
{
    if (matInfo.height != 0xffffffff)
    {
        Texture2D height = globalTextureData[matInfo.height];
        return height.Sample(inSampler, texCoords).r;
    }
    return 1.f;
}

#endif