#define HLSL
#include "../Common/TypesCompat.h"

#include "../Common/BRDF.hlsl"
#include "../GBuffer/GBuffer.hlsl"
#include "../Common/Math.hlsl"

Texture2D                           g_GBufferHeap[]                 : register(t5, space6);
TextureCube<float4>                 g_Cubemap                       : register(t7, space8);
  
SamplerState                        g_NearestRepeatSampler          : register(s0);
SamplerState                        g_LinearRepeatSampler           : register(s1);
SamplerState                        g_LinearClampSampler            : register(s2);

ConstantBuffer<DirectionalLightCB>  g_DirectionalLight              : register(b0);
ConstantBuffer<CameraCB>            g_Camera                        : register(b1);
    
struct PixelShaderOutput
{
    float4 DirectLight    : SV_TARGET0;
};

bool DirectLightGBuffer(in float2 texCoords, inout float4 radiance)
{
    GFragment fi = UnpackGBuffer(g_GBufferHeap, g_NearestRepeatSampler, texCoords);

    SurfaceMaterial currentMat;
    currentMat.albedo = fi.albedo;
    currentMat.ao = fi.ao;
    currentMat.emissive = fi.emissive;
    currentMat.height = fi.height;
    currentMat.metallic = fi.metallic;
    currentMat.roughness = fi.roughness;
    currentMat.normal = fi.normal;
    
    float3 X = GetWorldPosFromDepth(texCoords, fi.depth, g_Camera.invViewProj);
    X = OffsetRay(X, fi.geometryNormal) + fi.geometryNormal * 0.05f;
    
    float3 V = normalize(g_Camera.pos.rgb - X.rgb);
    float3 L = -g_DirectionalLight.direction.rgb;
    
    if (fi.shaderModel == SM_SKY)
    {
        radiance.rgb = SampleSky(-V, g_Cubemap, g_LinearRepeatSampler);
        return false;
    }
     
    radiance.rgb += EvaluateBRDF(currentMat.normal, L, V, currentMat) * g_DirectionalLight.color.w * g_DirectionalLight.color.rgb;
    
    return true;
}

PixelShaderOutput main(float2 TexCoord : TEXCOORD)
{
    PixelShaderOutput OUT;
    OUT.DirectLight = float4(0.f, 0.f, 0.f, 1.f);
    
    DirectLightGBuffer(TexCoord, OUT.DirectLight);  
    return OUT;
}