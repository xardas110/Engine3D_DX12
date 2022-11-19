#ifndef BRDF_H
#define BRDF_H
#include "MaterialAttributes.hlsl"
//Based on Raytracing Gems II chapter 14

#define MIN_F0 0.04f

#ifndef PI
#define PI 3.141592653589f
#endif

#ifndef TWO_PI
#define TWO_PI (2.0f * PI)
#endif

#ifndef ONE_OVER_PI
#define ONE_OVER_PI (1.0f / PI)
#endif

#ifndef ONE_OVER_TWO_PI
#define ONE_OVER_TWO_PI (1.0f / TWO_PI)
#endif

struct BRDFData
{
    float3 V;
    float LdotH;
    
    float3 N;
    float NdotH;
    
    float3 H;
    float VdotH;
    
    float3 L;
    float NdotL;
    
    float3 F;
    float NdotV;
    
    float3 specularF0;  
    float alpha;
    
    float3 diffuseReflectance;
    float alphaSquared; 
    
    float roughness;
    bool bVbackFacing;
    bool bLbackFacing;
};

float3 BaseColorToSpecularF0(float3 baseColor, float metallic)
{
    float3 F0 = float3(MIN_F0, MIN_F0, MIN_F0); 
    return lerp(F0, baseColor, metallic);
}

float3 BaseColorToDiffuseReflectance(float3 baseColor, float metallic)
{
    return baseColor * (1.f - metallic);
}

float3 EvaluateFresnelSchlick(in float3 f0, float f90, float NdotS)
{
    return f0 + (f90 - f0) * pow(1.0f - NdotS, 5.0f);
}

float Luminance(in float3 rgb)
{
    return dot(rgb, float3(0.2126f, 0.7152f, 0.0722f));
}

float ShadowedF90(in float3 F0)
{
    const float t = (1.f / MIN_F0);
    return min(1.0f, t * Luminance(F0));
}

BRDFData PrepareBRDFData(in float3 N, in float3 L, in float3 V, in SurfaceMaterial material)
{
    BRDFData data;
    
    data.V = V;
    data.N = N;
    data.H = normalize(L + V);
    data.L = L;
    
    float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    
    data.bLbackFacing = (NdotL <= 0.f);
    data.bVbackFacing = (NdotV <= 0.f);
    
    data.NdotL = min(max(0.00001f, NdotL), 1.0f);
    data.NdotV = min(max(0.00001f, NdotV), 1.0f);

    data.LdotH = saturate(dot(L, data.H));
    data.NdotH = saturate(dot(N, data.H));
    data.VdotH = saturate(dot(V, data.H));
    
    data.specularF0 = BaseColorToSpecularF0(material.albedo, material.metallic);
    data.diffuseReflectance = BaseColorToDiffuseReflectance(material.albedo, material.metallic);
    
    data.roughness = material.roughness;
    data.alpha = data.roughness * data.roughness;
    data.alphaSquared = data.alpha * data.alpha;

    data.F = EvaluateFresnelSchlick(data.specularF0, ShadowedF90(data.specularF0), data.LdotH);

    return data;
}

float GGX_D(float alphaSquared, float NdotH)
{
    float b = ((alphaSquared - 1.0f) * NdotH * NdotH + 1.0f);
    return alphaSquared / (PI * b * b);
}

float Smith_G2_Height_Correlated_GGX_Lagarde(float alphaSquared, float NdotL, float NdotV)
{
    float a = NdotV * sqrt(alphaSquared + NdotL * (NdotL - alphaSquared * NdotL));
    float b = NdotL * sqrt(alphaSquared + NdotV * (NdotV - alphaSquared * NdotV));
    return 0.5f / (a + b);
}

float Smith_G2(float alpha, float alphaSquared, float NdotL, float NdotV)
{
    return Smith_G2_Height_Correlated_GGX_Lagarde(alphaSquared, NdotL, NdotV);
}

float3 EvaluateMicrofacet(const in BRDFData data)
{
    float D = GGX_D(max(0.00001f, data.alphaSquared), data.NdotH);
    float G2 = Smith_G2(data.alpha, data.alphaSquared, data.NdotL, data.NdotV);
    return data.F * (G2 * D * data.NdotL);
}

float3 EvaluateLambertian(const in BRDFData data)
{
    return data.diffuseReflectance * (ONE_OVER_PI * data.NdotL);
}

float3 EvaluateBRDF(in float3 N, in float3 L, in float3 V, in SurfaceMaterial material)
{
    BRDFData data = PrepareBRDFData(N, L, V, material);

    if (data.bVbackFacing || data.bLbackFacing)
        return float3(0.0f, 0.0f, 0.0f);
    
    float3 specular = EvaluateMicrofacet(data);
    float3 diffuse = EvaluateLambertian(data);

    return (float3(1.0f, 1.0f, 1.0f) - data.F) * diffuse + specular;
}

#endif