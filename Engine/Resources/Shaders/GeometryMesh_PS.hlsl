
#ifndef GeometryMeshPS_HLSL
#define GeometryMeshPS_HLSL

#define HLSL
#include "RaytracingHlslCompat.h"

ConstantBuffer<ObjectCB>        g_ObjectCB                  : register(b0);

RaytracingAccelerationStructure Scene                       : register(t0);

Texture2D                       GlobalTextureData[]         : register(t1, space0);
StructuredBuffer<MeshVertex>    GlobalMeshVertexData[]      : register(t1, space1);

StructuredBuffer<MeshInfo>      GlobalMeshInfo              : register(t2, space2);
StructuredBuffer<MaterialInfo>  GlobalMaterialInfo          : register(t3, space3);

SamplerState                    LinearRepeatSampler         : register(s0);

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
    ray.Origin.y += 0.1f;
    ray.Direction = float3(0, 1, 0);
    query.TraceRayInline(Scene, ray_flags, ray_instance_mask, ray);
    
    // c. Cast 
    
    // Proceed() is where behind-the-scenes traversal happens, including the heaviest of any driver inlined code.
    // In this simplest of scenarios, Proceed() only needs to be called once rather than a loop.
    // Based on the template specialization above, traversal completion is guaranteed.
    
    query.Proceed();
    
    MeshInfo currentMesh = GlobalMeshInfo[g_ObjectCB.meshId];
    MaterialInfo currentMaterial = GlobalMaterialInfo[currentMesh.materialIndex];
    
    float4 texColor = GlobalTextureData[currentMaterial.albedo].Sample(LinearRepeatSampler, IN.TexCoord); //DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);
      
    if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        int instanceIndex = query.CommittedInstanceID();
        int primitiveIndex = query.CommittedPrimitiveIndex();
        int geometryIndex = query.CommittedGeometryIndex();
        
       // MeshInfo meshInfo = GlobalMeshInfo[instanceIndex];
       // MaterialInfo materialInfo = GlobalMaterialInfo[meshInfo.materialIndex];
        
        float4 albedo = float4(0.f, 0.f, 0.f, 1.f);
        /*
        if (materialInfo.albedo =! 0xffffffff)
        {
            albedo = GlobalTextureData[materialInfo.albedo].Sample(LinearRepeatSampler, IN.TexCoord);
        }
*/
                
        //StructuredBuffer<MeshVertex> meshVertex = GlobalMeshVertexData[3];
                
        texColor = albedo; //GlobalTextureArray[instanceIndex].Sample(LinearRepeatSampler, IN.TexCoord);

    }

   // float4 texColor = DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);
    return texColor;
}

#endif