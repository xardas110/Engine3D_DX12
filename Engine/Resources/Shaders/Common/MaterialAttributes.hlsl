#ifndef MATERIAL_ATTRIBUTES_H
#define MATERIAL_ATTRIBUTES_H

#define hlsl 
#include "TypesCompat.h"
#include "../Common/Math.hlsl"

struct SurfaceMaterial
{
    float3 albedo;
    float ao;
    
    float3 normal;
    float height;
      
    float3 emissive;
    float opacity;
    
    float roughness;
    float metallic;
};

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

float3 GetEmissive(in MaterialInfo matInfo, in float2 texCoords, in SamplerState inSampler, in Texture2D globalTextureData[])
{
    if (matInfo.emissive != 0xffffffff)
    {
        Texture2D emissive = globalTextureData[matInfo.emissive];
        return emissive.Sample(inSampler, texCoords).r;
    }
    return float3(1.f, 0.f, 0.f);
}

float GetOpacity(in MaterialInfo matInfo, in float2 texCoords, in SamplerState inSampler, in Texture2D globalTextureData[])
{
    if (matInfo.opacity != 0xffffffff)
    {
        Texture2D opacity = globalTextureData[matInfo.opacity];
        return opacity.Sample(inSampler, texCoords).r;
    }
    return 1.f;
}

SurfaceMaterial GetSurfaceMaterial(
    in MaterialInfo matInfo, in float2 texCoords, 
    in SamplerState inSampler, in Texture2D globalTextureData[])
{
    SurfaceMaterial surface;  
    surface.ao = GetAO(matInfo, texCoords, inSampler, globalTextureData);
    surface.albedo = GetAlbedo(matInfo, texCoords, inSampler, globalTextureData) * surface.ao;
    surface.emissive = GetEmissive(matInfo, texCoords, inSampler, globalTextureData);
    surface.height = GetHeight(matInfo, texCoords, inSampler, globalTextureData);
    surface.roughness = GetRoughness(matInfo, texCoords, inSampler, globalTextureData);
    surface.metallic = GetMetallic(matInfo, texCoords, inSampler, globalTextureData);
    surface.opacity = GetOpacity(matInfo, texCoords, inSampler, globalTextureData);
    surface.normal = GetNormal(matInfo, texCoords, inSampler, globalTextureData);
    return surface;
}

//Returns tangentNormal -> worldNormal
float3 TangentToWorldNormal(in float3 localTangent, in float3 localBiTangent, in float3 localNormal, in float3 tangentNormal, in float3x4 model)
{
    float3x3 TBN = ConstructTBN(model, localTangent, localBiTangent, normalize(localNormal));
    return GetWorldNormal(tangentNormal, TBN);
}

#endif