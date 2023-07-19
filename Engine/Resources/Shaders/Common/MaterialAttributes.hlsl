#ifndef MATERIAL_ATTRIBUTES_H
#define MATERIAL_ATTRIBUTES_H

#define hlsl 
#include "TypesCompat.h"
#include "Math.hlsl"

#define SAMPLE_TYPE_MIP 0
#define SAMPLE_TYPE_LEVEL 1
#define SAMPLE_TYPE_GRADIENT 2

struct SurfaceMaterial
{
    float3 albedo COMPAT_VEC3F(1.f, 0.f, 0.f);
    float ao COMPAT_FLOAT(1.f);
    
    float3 normal COMPAT_VEC3F(0.f, 1.f, 0.f);
    float height COMPAT_FLOAT(1.f);
      
    float3 emissive COMPAT_VEC3F(1.f, 0.f, 0.f);
    float opacity COMPAT_FLOAT(1.f);
    
    float3 transparent COMPAT_VEC3F(1.f, 1.f, 1.f);
    float roughness COMPAT_FLOAT(0.f);
    
    float3 specular COMPAT_VEC3F(1.f, 0.f, 0.f); //for phong shading model or PBR materials
    float metallic COMPAT_FLOAT(0.f);
};

//https://github.com/NVIDIAGameWorks/RayTracingDenoiser
float Pow(float x, float y)
{
    return pow(abs(x), y);
}

float2 Pow(float2 x, float y)
{
    return pow(abs(x), y);
}

float2 Pow(float2 x, float2 y)
{
    return pow(abs(x), y);
}

float3 Pow(float3 x, float y)
{
    return pow(abs(x), y);
}

float3 Pow(float3 x, float3 y)
{
    return pow(abs(x), y);
}

float4 Pow(float4 x, float y)
{
    return pow(abs(x), y);
}

float4 Pow(float4 x, float4 y)
{
    return pow(abs(x), y);
}

 // Pow for values in range [0; 1]
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

float3 LinearToSrgb(float3 color)
{
    const float4 consts = float4(1.055, 0.41666, -0.055, 12.92);
    color = saturate(color);

    return lerp(consts.x * Pow(color, consts.yyy) + consts.zzz, consts.w * color, color < 0.0031308);
}

float3 SrgbToLinear(float3 color)
{
    const float4 consts = float4(1.0 / 12.92, 1.0 / 1.055, 0.055 / 1.055, 2.4);
    color = saturate(color);

    return lerp(color * consts.x, Pow(color * consts.y + consts.zzz, consts.www), color > 0.04045);
}

float srgbToLinear(float srgbColor)
{
    if (srgbColor < 0.04045f)
        return srgbColor / 12.92f;
    else
        return float(pow((srgbColor + 0.055f) / 1.055f, 2.4f));
}

float3 srgbToLinear(float3 srgbColor)
{
    return float3(srgbToLinear(srgbColor.x), srgbToLinear(srgbColor.y), srgbToLinear(srgbColor.z));

}

float4 SampleTexture(Texture2D texture, SamplerState inSampler, float2 texCoords, float4 mipLevel, int sampleType)
{
    switch (sampleType)
    {
        case SAMPLE_TYPE_MIP:
            return texture.Sample(inSampler, texCoords, mipLevel.x);          
        case SAMPLE_TYPE_GRADIENT:
            return texture.SampleGrad(inSampler, texCoords, mipLevel.xy, mipLevel.zw);             
        case SAMPLE_TYPE_LEVEL:
        default:
            return texture.SampleLevel(inSampler, texCoords, mipLevel.x);                    
    }
}

float3 GetNormal(in MaterialInfoGPU matInfo, in float2 texCoords, out bool bMatHasNormal, in SamplerState inSampler, in Texture2D globalTextureData[], float4 mipLevel = 0.f, int sampleType = SAMPLE_TYPE_LEVEL)
{
    Texture2D normal = globalTextureData[matInfo.normal];
    bMatHasNormal = true;
        
    if (sampleType == SAMPLE_TYPE_MIP)
        return normal.Sample(inSampler, texCoords, mipLevel.x);
    else if (sampleType == SAMPLE_TYPE_GRADIENT)
        return normal.SampleGrad(inSampler, texCoords, mipLevel.xy, mipLevel.zw);
            
    return normal.SampleLevel(inSampler, texCoords, mipLevel.x);
}

float3 GetSpecular(in MaterialInfoGPU matInfo, in float2 texCoords, out bool bMatHasSpecular, in SamplerState inSampler, in Texture2D globalTextureData[], float4 mipLevel = 0.f, int sampleType = SAMPLE_TYPE_LEVEL)
{
    Texture2D specular = globalTextureData[matInfo.specular];
    bMatHasSpecular = true;
        
    if (sampleType == SAMPLE_TYPE_MIP)
        return specular.Sample(inSampler, texCoords, mipLevel.x);
    else if (sampleType == SAMPLE_TYPE_GRADIENT)
        return specular.SampleGrad(inSampler, texCoords, mipLevel.xy, mipLevel.zw);
            
    return specular.SampleLevel(inSampler, texCoords, mipLevel.x);
}

float GetAO(in MaterialInfoGPU matInfo, in float2 texCoords, in SamplerState inSampler, in Texture2D globalTextureData[], float4 mipLevel = 0.f, int sampleType = SAMPLE_TYPE_LEVEL)
{
    Texture2D ao = globalTextureData[matInfo.ao];
        
    if (sampleType == SAMPLE_TYPE_MIP)
        return ao.Sample(inSampler, texCoords, mipLevel.x);
    else if (sampleType == SAMPLE_TYPE_GRADIENT)
        return ao.SampleGrad(inSampler, texCoords, mipLevel.xy, mipLevel.zw);
            
    return ao.SampleLevel(inSampler, texCoords, mipLevel.x);
}

float GetRoughness(in MaterialInfoGPU matInfo, in float2 texCoords, in SamplerState inSampler, in Texture2D globalTextureData[], float4 mipLevel = 0.f, int sampleType = SAMPLE_TYPE_LEVEL)
{
    Texture2D roughness = globalTextureData[matInfo.roughness];
        
    if (sampleType == SAMPLE_TYPE_MIP)
        return roughness.Sample(inSampler, texCoords, mipLevel.x);
    else if (sampleType == SAMPLE_TYPE_GRADIENT)
        return roughness.SampleGrad(inSampler, texCoords, mipLevel.xy, mipLevel.zw);
            
    return roughness.SampleLevel(inSampler, texCoords, mipLevel.x);
}

float GetMetallic(in MaterialInfoGPU matInfo, in float2 texCoords, in SamplerState inSampler, in Texture2D globalTextureData[], float4 mipLevel = 0.f, int sampleType = SAMPLE_TYPE_LEVEL)
{
    Texture2D metallic = globalTextureData[matInfo.metallic];
        
    if (sampleType == SAMPLE_TYPE_MIP)
        return metallic.Sample(inSampler, texCoords, mipLevel.x);
    else if (sampleType == SAMPLE_TYPE_GRADIENT)
        return metallic.SampleGrad(inSampler, texCoords, mipLevel.xy, mipLevel.zw);
            
    return metallic.SampleLevel(inSampler, texCoords, mipLevel.x);
}

float GetHeight(in MaterialInfoGPU matInfo, in float2 texCoords, in SamplerState inSampler, in Texture2D globalTextureData[], float4 mipLevel = 0.f, int sampleType = SAMPLE_TYPE_LEVEL)
{
    Texture2D height = globalTextureData[matInfo.height];
        
    if (sampleType == SAMPLE_TYPE_MIP)
        return height.Sample(inSampler, texCoords, mipLevel.x);
    else if (sampleType == SAMPLE_TYPE_GRADIENT)
        return height.SampleGrad(inSampler, texCoords, mipLevel.xy, mipLevel.zw);
            
    return height.SampleLevel(inSampler, texCoords, mipLevel.x);
}

float3 GetEmissive(in MaterialInfoGPU matInfo, in float2 texCoords, in SamplerState inSampler, in Texture2D globalTextureData[], float4 mipLevel = 0.f, int sampleType = SAMPLE_TYPE_LEVEL)
{
    Texture2D emissive = globalTextureData[matInfo.emissive];
        
    if (sampleType == SAMPLE_TYPE_MIP)
        return emissive.Sample(inSampler, texCoords, mipLevel.x);
    else if (sampleType == SAMPLE_TYPE_GRADIENT)
        return emissive.SampleGrad(inSampler, texCoords, mipLevel.xy, mipLevel.zw);
            
    return emissive.SampleLevel(inSampler, texCoords, mipLevel.x);
}

float GetOpacity(in MaterialInfoGPU matInfo, in float2 texCoords, in SamplerState inSampler, in Texture2D globalTextureData[], float4 mipLevel = 0.f, int sampleType = SAMPLE_TYPE_LEVEL)
{
    Texture2D opacity = globalTextureData[matInfo.opacity];
        
    if (sampleType == SAMPLE_TYPE_MIP)
        return opacity.Sample(inSampler, texCoords, mipLevel.x);
    else if (sampleType == SAMPLE_TYPE_GRADIENT)
        return opacity.SampleGrad(inSampler, texCoords, mipLevel.xy, mipLevel.zw);
            
    return opacity.SampleLevel(inSampler, texCoords, mipLevel.x);
}

//Returns tangentNormal -> worldNormal
float3 TangentToWorldNormal(in float3 localTangent, in float3 localBiTangent, in float3 localNormal, in float3 tangentNormal, in float3x4 model)
{
    float3x3 TBN = ConstructTBN(model, localTangent, localBiTangent, normalize(localNormal));
    return GetWorldNormal(tangentNormal, TBN);
}

SurfaceMaterial GetSurfaceMaterial(
    in MaterialInfoGPU matInfo, in MeshVertex v, in float3x4 model, in float4 objRotQuat,
    in SamplerState inSampler, in Texture2D globalTextureData[], float4 mipLevel = 0.f, int sampleType = SAMPLE_TYPE_LEVEL)
{
    SurfaceMaterial surface;    
    //surface.ao = GetAO(matInfo, v.textureCoordinate, inSampler, globalTextureData, mipLevel, sampleType);

    surface.albedo = SrgbToLinear(SampleTexture(globalTextureData[matInfo.albedo], inSampler, v.textureCoordinate, mipLevel, sampleType));
    surface.emissive = SampleTexture(globalTextureData[matInfo.emissive], inSampler, v.textureCoordinate, mipLevel, sampleType);
    surface.height = SampleTexture(globalTextureData[matInfo.height], inSampler, v.textureCoordinate, mipLevel, sampleType);
    surface.roughness = SampleTexture(globalTextureData[matInfo.roughness], inSampler, v.textureCoordinate, mipLevel, sampleType);
    surface.metallic = SampleTexture(globalTextureData[matInfo.metallic], inSampler, v.textureCoordinate, mipLevel, sampleType);
    surface.opacity = SampleTexture(globalTextureData[matInfo.opacity], inSampler, v.textureCoordinate, mipLevel, sampleType);
    surface.specular = SampleTexture(globalTextureData[matInfo.specular], inSampler, v.textureCoordinate, mipLevel, sampleType);
    surface.normal = SampleTexture(globalTextureData[matInfo.normal], inSampler, v.textureCoordinate, mipLevel, sampleType);
    surface.normal = TangentToWorldNormal(v.tangent, v.bitangent, v.normal, surface.normal, model);
    
    //TODO: Add flag if the material has no normal to use vertex normal instead
    //surface.normal = RotatePoint(objRotQuat, v.normal);

    return surface;
}

void ApplyMaterial(in MaterialInfoGPU matInfo, inout SurfaceMaterial surfaceMat, in MaterialColor materialColor)
{
    surfaceMat.albedo = surfaceMat.albedo * materialColor.diffuse.rgb;
    surfaceMat.emissive = surfaceMat.emissive * materialColor.emissive;
    surfaceMat.roughness = surfaceMat.roughness * materialColor.roughness;
    surfaceMat.metallic = surfaceMat.metallic * materialColor.metallic;  
}

#endif