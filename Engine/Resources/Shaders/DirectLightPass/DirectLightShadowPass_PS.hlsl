#define HLSL
#include "../Common/TypesCompat.h"

#include "..\..\..\Libs\RayTracingDenoiser\Shaders\Include\NRDEncoding.hlsli"

#define NRD_HEADER_ONLY
#include "..\..\..\Libs\RayTracingDenoiser\Shaders\Include\NRD.hlsli"

#include "../Common/BRDF.hlsl"
#include "../GBuffer/GBuffer.hlsl"
#include "../Common/MaterialAttributes.hlsl"
#include "../Common/RaytracingHelper.hlsl"

RaytracingAccelerationStructure             g_Scene                     : register(t0);
Texture2D                                   g_GlobalTextureData[]       : register(t1, space0);
StructuredBuffer<MeshVertex>                g_GlobalMeshVertexData[]    : register(t1, space1);
StructuredBuffer<uint>                      g_GlobalMeshIndexData[]     : register(t1, space2);

StructuredBuffer<MeshInfo>                  g_GlobalMeshInfo            : register(t2, space3);
StructuredBuffer<MaterialInfo>              g_GlobalMaterialInfo        : register(t3, space4);
StructuredBuffer<Material>                  g_GlobalMaterials           : register(t4, space5);

Texture2D                                   g_GBufferHeap[]             : register(t5, space6);

SamplerState                                g_NearestRepeatSampler      : register(s0);
SamplerState                                g_LinearRepeatSampler       : register(s1);
SamplerState                                g_LinearClampSampler        : register(s2);

ConstantBuffer<DirectionalLightCB>          g_DirectionalLight          : register(b0);
ConstantBuffer<CameraCB>                    g_Camera                    : register(b1);
ConstantBuffer<RaytracingDataCB>            g_RaytracingData            : register(b2);
    
struct PixelShaderOutput
{
    float visibility    : SV_TARGET0;
};

PixelShaderOutput main(float2 TexCoord : TEXCOORD)
{
    PixelShaderOutput OUT;

    GFragment fi = UnpackGBuffer(g_GBufferHeap, g_NearestRepeatSampler, TexCoord);       
    OUT.visibility = 1.f;
    return OUT;
}