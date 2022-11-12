
#ifndef GeometryMeshPS_HLSL
#define GeometryMeshPS_HLSL

#define HLSL
#include "RaytracingHlslCompat.h"

ConstantBuffer<ObjectCB> g_ObjectCB : register(b0);

RaytracingAccelerationStructure Scene : register(t0);
Texture2D GlobalTextureArray[] : register(t1, space0);
StructuredBuffer<MeshVertex> GlobalMeshVertexData[] : register(t1, space1);

SamplerState LinearRepeatSampler : register(s0);

struct PixelShaderInput
{
    float4 PositionWS : POSITION;
    float3 NormalWS : NORMAL;
    float2 TexCoord : TEXCOORD;
};

float4 main(PixelShaderInput IN) : SV_Target
{
    
    RayQuery < RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > query;
            
    uint ray_flags = 0; // Any this ray requires in addition those above.
    uint ray_instance_mask = 0xffffffff;

    // b. Initialize  - hardwired here to deliver minimal sample code.
    RayDesc ray;
    ray.TMin = 1e-5f;
    ray.TMax = 1e10f;
    ray.Origin = IN.PositionWS.xyz;
    ray.Direction = float3(0, 1, 0);
    query.TraceRayInline(Scene, ray_flags, ray_instance_mask, ray);
    
    // c. Cast 
    
    // Proceed() is where behind-the-scenes traversal happens, including the heaviest of any driver inlined code.
    // In this simplest of scenarios, Proceed() only needs to be called once rather than a loop.
    // Based on the template specialization above, traversal completion is guaranteed.
    
    query.Proceed();
    float4 texColor = GlobalTextureArray[g_ObjectCB.textureId].Sample(LinearRepeatSampler, IN.TexCoord); //DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);
      
    if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        int instanceIndex = query.CommittedInstanceID();
        int primitiveIndex = query.CommittedPrimitiveIndex();
        int geometryIndex = query.CommittedGeometryIndex();
        
        StructuredBuffer<MeshVertex> meshVertex = GlobalMeshVertexData[3];
        
        
        texColor = float4(meshVertex[0].position.x, meshVertex[0].position.y, meshVertex[0].position.z, 1.f); //GlobalTextureArray[instanceIndex].Sample(LinearRepeatSampler, IN.TexCoord);

    }

   // float4 texColor = DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);
    return texColor;
}

#endif