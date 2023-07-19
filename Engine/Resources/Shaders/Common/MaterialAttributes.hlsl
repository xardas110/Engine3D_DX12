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
    float3 albedo COMPAT_VEC3F(1.f, 1.f, 1.f);
    float ao COMPAT_FLOAT(1.f);
    
    float3 normal COMPAT_VEC3F(0.f, 1.f, 0.f);
    float height COMPAT_FLOAT(1.f);
      
    float3 emissive COMPAT_VEC3F(1.f, 1.f, 1.f);
    float opacity COMPAT_FLOAT(1.f);
    
    float3 transparent COMPAT_VEC3F(1.f, 1.f, 1.f);
    float roughness COMPAT_FLOAT(1.f);
    
    float3 specular COMPAT_VEC3F(1.f, 0.f, 0.f); //for phong shading model or PBR materials
    float metallic COMPAT_FLOAT(1.f);
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

// Helper function to sample texture based on sample type.
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

// Helper function to sample texture if valid, else return a default value.
// White low res textures could have been used, however this is faster due to texture lookups being very expensive.
float4 SampleValidTexture(Texture2D textureArray[], SamplerState inSampler, uint texID, float2 texCoords, float4 mipLevel, int sampleType)
{
    if (texID == 0xffffffff)
        return 1.0f;
    
    return SampleTexture(textureArray[texID], inSampler, texCoords, mipLevel, sampleType);
}

// Converts tangent space normals to world space normals.
float3 TangentToWorldNormal(in float3 localTangent, in float3 localBiTangent, in float3 localNormal, in float3 tangentNormal, in float3x4 model)
{
    float3x3 TBN = ConstructTBN(model, localTangent, localBiTangent, normalize(localNormal));
    return GetWorldNormal(tangentNormal, TBN);
}

// Computes the surface material from texture samples.
SurfaceMaterial GetSurfaceMaterial(
    in MaterialInfoGPU matInfo,
    in MeshVertex v,
    in float3x4 model,
    in float4 objRotQuat,
    in SamplerState inSampler,
    in Texture2D globalTextureData[],
    float4 mipLevel = 0.f,
    int sampleType = SAMPLE_TYPE_LEVEL)
{
    SurfaceMaterial surface;
    
    surface.albedo = (matInfo.albedo == 0xffffffff) ? 1.0f : SrgbToLinear(SampleValidTexture(globalTextureData, inSampler, matInfo.albedo, v.textureCoordinate, mipLevel, sampleType));
    surface.emissive = SampleValidTexture(globalTextureData, inSampler, matInfo.emissive, v.textureCoordinate, mipLevel, sampleType);
    surface.height = SampleValidTexture(globalTextureData, inSampler, matInfo.height, v.textureCoordinate, mipLevel, sampleType);
    surface.roughness = SampleValidTexture(globalTextureData, inSampler, matInfo.roughness, v.textureCoordinate, mipLevel, sampleType);
    surface.metallic = SampleValidTexture(globalTextureData, inSampler, matInfo.metallic, v.textureCoordinate, mipLevel, sampleType);
    surface.opacity = SampleValidTexture(globalTextureData, inSampler, matInfo.opacity, v.textureCoordinate, mipLevel, sampleType);
    surface.specular = SampleValidTexture(globalTextureData, inSampler, matInfo.specular, v.textureCoordinate, mipLevel, sampleType);

    surface.normal = (matInfo.normal == 0xffffffff) ? RotatePoint(objRotQuat, v.normal) : TangentToWorldNormal(v.tangent, v.bitangent, v.normal, SampleTexture(globalTextureData[matInfo.normal], inSampler, v.textureCoordinate, mipLevel, sampleType), model);

    return surface;
}

// Modulates surface material properties with given material colors.
void ApplyMaterial(in MaterialInfoGPU matInfo, inout SurfaceMaterial surfaceMat, in MaterialColor materialColor)
{
    surfaceMat.albedo *= materialColor.diffuse.rgb;
    surfaceMat.emissive *= materialColor.emissive;
    surfaceMat.roughness *= materialColor.roughness;
    surfaceMat.metallic *= materialColor.metallic;
}
#endif