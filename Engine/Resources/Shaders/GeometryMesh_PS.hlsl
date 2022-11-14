
#ifndef GeometryMeshPS_HLSL
#define GeometryMeshPS_HLSL

#define HLSL
#include "RaytracingHlslCompat.h"
#include "Quaternion.hlsl"

ConstantBuffer<ObjectCB>        g_ObjectCB                  : register(b0);

RaytracingAccelerationStructure Scene                       : register(t0);

Texture2D                       GlobalTextureData[]         : register(t1, space0);
StructuredBuffer<MeshVertex>    GlobalMeshVertexData[]      : register(t1, space1);
StructuredBuffer<uint> GlobalMeshIndexData[] : register(t1, space2);

StructuredBuffer<MeshInfo>      GlobalMeshInfo              : register(t2, space3);
StructuredBuffer<MaterialInfo>  GlobalMaterialInfo          : register(t3, space4);

SamplerState                    LinearRepeatSampler         : register(s0);

struct PixelShaderInput
{
    float4 PositionWS : POSITION;
    float3 NormalWS : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct HitAttributes
{
    float2 barycentrics;
    int primitiveIndex;
    int geometryIndex;
    float3x4 objToWorld;
    bool bFrontFaced;
};

float2 BaryInterp2(in float2 v0, in float2 v1, in float2 v2, in float3 bary)
{
    float2 r;
    
    float u = bary.x;
    float v = bary.y;
    float w = bary.z;
    
    r.x = u * v0.x + v * v1.x + w * v2.x;
    r.y = u * v0.y + v * v1.y + w * v2.y;
    
    return r;
}

float3 BaryInterp3(in float3 v0, in float3 v1, in float3 v2, in float3 bary)
{
    //X = (u * p0.x + v * p1.x + w * p2.x)
    //Y = (u * p0.y + v * p1.y + w * p2.y)
    //Z = (u * p0.z + v * p1.z + w * p2.z)
    
    float3 r;
    float u = bary.x;
    float v = bary.y;
    float w = bary.z;
    
    r.x = bary.x * v0.x + bary.y * v1.x + bary.z * v2.x;
    r.y = bary.x * v0.y + bary.y * v1.y + bary.z * v2.y;
    r.z = bary.x * v0.z + bary.y * v1.z + bary.z * v2.z;
    return r;
}

MeshVertex BarycentricLerp(in MeshVertex v0, in MeshVertex v1, in MeshVertex v2, in float3 bary)
{
    MeshVertex r;
    r.position = BaryInterp3(v0.position, v1.position, v2.position, bary);
    r.normal = BaryInterp3(v0.normal, v1.normal, v2.normal, bary);
    r.textureCoordinate = BaryInterp2(v0.textureCoordinate, v1.textureCoordinate, v2.textureCoordinate, bary);
    return r;
}

MeshVertex GetHitSurface(in HitAttributes attr, in MeshInfo meshInfo)
{
    float3 bary;
    bary.x = 1 - attr.barycentrics.x - attr.barycentrics.y;
    bary.y = attr.barycentrics.x;
    bary.z = attr.barycentrics.y;
    
    StructuredBuffer<MeshVertex> vertexBuffer = GlobalMeshVertexData[meshInfo.vertexOffset];
    StructuredBuffer<uint> indexBuffer = GlobalMeshIndexData[meshInfo.indexOffset];

    const uint primId = attr.primitiveIndex;
    const uint i0 = indexBuffer[primId * 3 + 0];
    const uint i1 = indexBuffer[primId * 3 + 1];
    const uint i2 = indexBuffer[primId * 3 + 2];
   
    MeshVertex v0 = vertexBuffer[i0];
    MeshVertex v1 = vertexBuffer[i1];
    MeshVertex v2 = vertexBuffer[i2];
    
    MeshVertex result = BarycentricLerp(v0, v1, v2, bary);
    result.normal = rotate_vector(result.normal, meshInfo.objRot);

    return result;
}

float4 main(PixelShaderInput IN) : SV_Target
{
    
    RayQuery < RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > query;
            
    uint ray_flags = 0; // Any this ray requires in addition those above.
    uint ray_instance_mask = 0xffffffff;

    // b. Initialize  - hardwired here to deliver minimal sample code.
    RayDesc ray;
    ray.TMin = 0.01f;
    ray.TMax = 1e10f;
    ray.Origin = IN.PositionWS.xyz;
    //ray.Origin += IN.NormalWS * 0.2f;
    ray.Direction = normalize(float3(-1, 0, -1));
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
        HitAttributes hit;
        
        int instanceIndex = query.CommittedInstanceID();
        hit.primitiveIndex = query.CommittedPrimitiveIndex();
        hit.geometryIndex = query.CommittedGeometryIndex();
        hit.bFrontFaced = query.CommittedTriangleFrontFace();
        hit.barycentrics = query.CommittedTriangleBarycentrics();
        hit.objToWorld = query.CommittedObjectToWorld3x4();
        
        MeshInfo meshInfo = GlobalMeshInfo[instanceIndex];
        MaterialInfo materialInfo = GlobalMaterialInfo[meshInfo.materialIndex];
        
        //StructuredBuffer<MeshVertex> meshVertices = GlobalMeshVertexData[meshInfo.vertexOffset];
        //MeshVertex meshVertex = meshVertices[hit.primitiveIndex];
       
        if (hit.bFrontFaced == true)
        {       
            float4 albedo = float4(hit.barycentrics.x, hit.barycentrics.y, 0.f, 1.f);
        
            MeshVertex hitSurface = GetHitSurface(hit, meshInfo);
            albedo = float4(hitSurface.normal.x, hitSurface.normal.y, hitSurface.normal.z, 1.f);
            //albedo = GlobalTextureData[materialInfo.albedo].Sample(LinearRepeatSampler, hitSurface.textureCoordinate);
                       
            //StructuredBuffer<MeshVertex> meshVertex = GlobalMeshVertexData[3];
                
            texColor = albedo; //GlobalTextureArray[instanceIndex].Sample(LinearRepeatSampler, IN.TexCoord);
        }
    }

    //texColor = float4(IN.NormalWS, 1.f);
    
   // float4 texColor = DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);
    return texColor;
}

#endif