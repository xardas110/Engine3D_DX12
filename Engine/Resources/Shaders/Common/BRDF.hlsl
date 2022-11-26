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

#define BRDF_TYPE_DIFFUSE 1
#define BRDF_TYPE_SPECULAR 2

#define RngStateType uint4

#define SpecularMaskingFunctionSmithGGXCorrelated

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

float3 EvaluateDiffuseBRDF(in BRDFData data)
{
    if (data.bVbackFacing || data.bLbackFacing)
        return float3(0.0f, 0.0f, 0.0f);
    
    return (float3(1.0f, 1.0f, 1.0f) - data.F) * EvaluateLambertian(data);
}

float3 EvaluateSpecularBRDF(in BRDFData data)
{
    if (data.bVbackFacing || data.bLbackFacing)
        return float3(0.0f, 0.0f, 0.0f);
    
    return EvaluateMicrofacet(data);
}

// Source: "Hash Functions for GPU Rendering" by Jarzynski & Olano
uint4 pcg4d(uint4 v)
{
    v = v * 1664525u + 1013904223u;

    v.x += v.y * v.w;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.w += v.y * v.z;

    v = v ^ (v >> 16u);

    v.x += v.y * v.w;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.w += v.y * v.z;

    return v;
}

RngStateType InitRNG(uint2 pixelCoords, uint2 resolution, uint frameNumber)
{
    return RngStateType(pixelCoords.xy, frameNumber, 0);
}

float uintToFloat(uint x)
{
    return asfloat(0x3f800000 | (x >> 9)) - 1.0f;
}

// Return random float in <0; 1) range  (PCG version)
float Rand(inout RngStateType rngState)
{
    rngState.w++; //< Increment sample index
    return uintToFloat(pcg4d(rngState).x);
}

float GetBRDFProbability(in SurfaceMaterial material, in float3 V, in float3 shadingNormal)
{
    float specularF0 = Luminance(BaseColorToSpecularF0(material.albedo, material.metallic));
    float diffuseReflectance = Luminance(BaseColorToDiffuseReflectance(material.albedo, material.metallic));
    float Fresnel = saturate(Luminance(EvaluateFresnelSchlick(specularF0, ShadowedF90(specularF0), max(0.0f, dot(V, shadingNormal)))));
    
    float specular = Fresnel;
    float diffuse = diffuseReflectance * (1.0f - Fresnel);
    float p = (specular / max(0.0001f, (specular + diffuse)));
    return clamp(p, 0.1f, 0.9f);
}

float4 GetRotationToZAxis(in float3 input)
{

	// Handle special case when input is exact or near opposite of (0, 0, 1)
    if (input.z < -0.99999f)
        return float4(1.0f, 0.0f, 0.0f, 0.0f);

    return normalize(float4(input.y, -input.x, 0.0f, 1.0f + input.z));
}

// Source: https://gamedev.stackexchange.com/questions/28395/rotating-vector3-by-a-quaternion
float3 RotatePoint(float4 q, float3 v)
{
    const float3 qAxis = float3(q.x, q.y, q.z);
    return 2.0f * dot(qAxis, v) * qAxis + (q.w * q.w - dot(qAxis, qAxis)) * v + 2.0f * q.w * cross(qAxis, v);
}

float3 SampleHemisphere(float2 u, out float pdf) 
{
    float a = sqrt(u.x);
    float b = TWO_PI * u.y;

    float3 result = float3(
		    a * cos(b),
		    a * sin(b),
		    sqrt(1.0f - u.x));

	pdf = result.z * ONE_OVER_PI;

	return result;
}

float3 SampleHemisphere(float2 u)
{
    float pdf;
    return SampleHemisphere(u, pdf);
}

float Lambertian(const in BRDFData data)
{
    return 1.0f;
}

float3 SampleGGXVNDF(float3 Ve, float2 alpha2D, float2 u)
{
    float3 Vh = normalize(float3(alpha2D.x * Ve.x, alpha2D.y * Ve.y, Ve.z));

    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    float3 T1 = lensq > 0.0f ? float3(-Vh.y, Vh.x, 0.0f) * rsqrt(lensq) : float3(1.0f, 0.0f, 0.0f);
    float3 T2 = cross(Vh, T1);

    float r = sqrt(u.x);
    float phi = TWO_PI * u.y;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5f * (1.0f + Vh.z);
    t2 = lerp(sqrt(1.0f - t1 * t1), t2, s);

    float3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0f, 1.0f - t1 * t1 - t2 * t2)) * Vh;
	
    return normalize(float3(alpha2D.x * Nh.x, alpha2D.y * Nh.y, max(0.0f, Nh.z)));
}

float Smith_G1_GGX(float a)
{
    float a2 = a * a;
    return 2.0f / (sqrt((a2 + 1.0f) / a2) + 1.0f);
}

float Smith_G1_GGX(float alpha, float NdotS, float alphaSquared, float NdotSSquared)
{
    return 2.0f / (sqrt(((alphaSquared * (1.0f - NdotSSquared)) + NdotSSquared) / NdotSSquared) + 1.0f);
}

float Smith_G2_Over_G1_Height_Correlated(float alpha, float alphaSquared, float NdotL, float NdotV)
{
    float G1V = Smith_G1_GGX(alpha, NdotV, alphaSquared, NdotV * NdotV);
    float G1L = Smith_G1_GGX(alpha, NdotL, alphaSquared, NdotL * NdotL);
    return G1L / (G1V + G1L - G1V * G1L);
}

float SpecularSampleWeightGGXVNDF(float alpha, float alphaSquared, float NdotL, float NdotV, float HdotL, float NdotH)
{
	return Smith_G2_Over_G1_Height_Correlated(alpha, alphaSquared, NdotL, NdotV);
}

float3 SampleSpecularMicrofacet(float3 Vlocal, float alpha, float alphaSquared, float3 specularF0, float2 u, out float3 weight) 
{
	// Sample a microfacet normal (H) in local space
    float3 Hlocal;
	if (alpha == 0.0f) 
    {
		// Fast path for zero roughness (perfect reflection), also prevents NaNs appearing due to divisions by zeroes
		Hlocal = float3(0.0f, 0.0f, 1.0f);
	} 
    else 
    {
		// For non-zero roughness, this calls VNDF sampling for GG-X distribution or Walter's sampling for Beckmann distribution
		Hlocal = SampleGGXVNDF(Vlocal, float2(alpha, alpha), u);
	}

	// Reflect view direction to obtain light vector
    float3 Llocal = reflect(-Vlocal, Hlocal);

    float HdotL = max(0.00001f, min(1.0f, dot(Hlocal, Llocal)));
    const float3 Nlocal = float3(0.0f, 0.0f, 1.0f);
    float NdotL = max(0.00001f, min(1.0f, dot(Nlocal, Llocal)));
    float NdotV = max(0.00001f, min(1.0f, dot(Nlocal, Vlocal)));
    float NdotH = max(0.00001f, min(1.0f, dot(Nlocal, Hlocal)));
    float3 F = EvaluateFresnelSchlick(specularF0, ShadowedF90(specularF0), HdotL);

    weight = F * SpecularSampleWeightGGXVNDF(alpha, alphaSquared, NdotL, NdotV, HdotL, NdotH);

    return Llocal;
}

float4 InvertRotation(float4 q)
{
    return float4(-q.x, -q.y, -q.z, q.w);
}

bool EvaluateIndirectBRDF(
    in float2 u, in float3 shadingNormal, in float3 geometryNormal, in float3 V, 
    in SurfaceMaterial material, const int brdfType, 
    out float3 rayDirection, out float3 sampleWeight)
{
    if (dot(shadingNormal, V) <= 0.0f)
        return false;
    
    float4 qRotationToZ = GetRotationToZAxis(shadingNormal);
    float3 Vlocal = RotatePoint(qRotationToZ, V);
    const float3 Nlocal = float3(0.0f, 0.0f, 1.0f);
    
    float3 rayDirectionLocal = float3(0.0f, 0.0f, 0.0f);
    
    if (BRDF_TYPE_DIFFUSE == brdfType)
    {
        rayDirectionLocal = SampleHemisphere(u);
        const BRDFData data = PrepareBRDFData(Nlocal, rayDirectionLocal, Vlocal, material);
        sampleWeight = data.diffuseReflectance * Lambertian(data);
        
        //TODO:Generate new u value
        float3 hSpecular = SampleGGXVNDF(Vlocal, float2(data.alpha, data.alpha), u);
        
        float VdotH = max(0.00001f, min(1.0f, dot(Vlocal, hSpecular)));
        sampleWeight *= (float3(1.0f, 1.0f, 1.0f) - EvaluateFresnelSchlick(data.specularF0, ShadowedF90(data.specularF0), VdotH));
    }
    else if (BRDF_TYPE_SPECULAR == brdfType)
    {
        const BRDFData data = PrepareBRDFData(Nlocal, float3(0.0f, 0.0f, 1.0f) /* unused L vector */, Vlocal, material);
        rayDirectionLocal = SampleSpecularMicrofacet(Vlocal, data.alpha, data.alphaSquared, data.specularF0, u, sampleWeight);
    }

    if (Luminance(sampleWeight) == 0.0f)
        return false;
    
    rayDirection = normalize(RotatePoint(InvertRotation(qRotationToZ), rayDirectionLocal));
    
    if (dot(geometryNormal, rayDirection) <= 0.0f)
        return false;

    return true;
}

//Raytracing gems chapter 6
float3 OffsetRay(const float3 p, const float3 n)
{
    static const float origin = 1.0f / 32.0f;
    static const float float_scale = 1.0f / 65536.0f;
    static const float int_scale = 256.0f;

    int3 of_i = int3(int_scale * n.x, int_scale * n.y, int_scale * n.z);

    float3 p_i = float3(
		asfloat(asint(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
		asfloat(asint(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
		asfloat(asint(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));

    return float3(abs(p.x) < origin ? p.x + float_scale * n.x : p_i.x,
		abs(p.y) < origin ? p.y + float_scale * n.y : p_i.y,
		abs(p.z) < origin ? p.z + float_scale * n.z : p_i.z);
}

//https://github.com/NVIDIAGameWorks/Falcor/blob/056f7b7c73b69fa8140d211bbf683ddf297a2ae0/Source/Falcor/Rendering/Materials/Microfacet.slang#L213
/** Approximate pre-integrated specular BRDF. The approximation assumes GGX VNDF and Schlick's approximation.
    See Eq 4 in [Ray Tracing Gems, Chapter 32]

    \param[in] specularReflectance Reflectance from a direction parallel to the normal.
    \param[in] alpha GGX width parameter (should be clamped to small epsilon beforehand).
    \param[in] cosTheta Dot product between shading normal and evaluated direction, in the positive hemisphere.
*/
float3 ApproxSpecularIntegralGGX(float3 specularReflectance, float alpha, float cosTheta)
{
    cosTheta = abs(cosTheta);

    float4 X;
    X.x = 1.f;
    X.y = cosTheta;
    X.z = cosTheta * cosTheta;
    X.w = cosTheta * X.z;
 
    float4 Y;
    Y.x = 1.f;
    Y.y = alpha;
    Y.z = alpha * alpha;
    Y.w = alpha * Y.z;
      
    float2x2 M1 = float2x2(
        0.995367f, -1.38839f,
        -0.24751f, 1.97442f
    );

    float3x3 M2 = float3x3(
        1.0f, 2.68132f, 52.366f,
        16.0932f, -3.98452f, 59.3013f,
        -5.18731f, 255.259f, 2544.07f
    );

    float2x2 M3 = float2x2(
        -0.0564526f, 3.82901f,
        16.91f, -11.0303f
    );

    float3x3 M4 = float3x3(
        1.0f, 4.11118f, -1.37886f,
        19.3254f, -28.9947f, 16.9514f,
        0.545386f, 96.0994f, -79.4492f
    );
     
    float bias = dot(mul(M1, X.xy), Y.xy) * rcp(dot(mul(M2, X.xyw), Y.xyw));
    float scale = dot(mul(M3, X.xy), Y.xy) * rcp(dot(mul(M4, X.xzw), Y.xyw));
    
    // This is a hack for specular reflectance of 0
    float specularReflectanceLuma = dot(specularReflectance, float3(1.f / 3.f, 1.f / 3.f, 1.f / 3.f));

    bias *= saturate(specularReflectanceLuma * 50.0f);
  
    return mad(specularReflectance, max(0.0, scale), max(0.0, bias));
}

float4 SampleSky(in float3 dir, in TextureCube<float4> cubemap, in SamplerState linearSampler)
{
    return cubemap.Sample(linearSampler, dir);
}

#endif