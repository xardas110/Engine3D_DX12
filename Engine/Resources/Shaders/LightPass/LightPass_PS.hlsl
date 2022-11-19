#define HLSL
#include "../Common/TypesCompat.h"
#include "../GBuffer/GBuffer.hlsl" 
#include "../Common/Math.hlsl"
#include "../Common/Phong.hlsl"
#include "../Common/RaytracingHelper.hlsl"
#include "../Common/MaterialAttributes.hlsl"
#include "../Common/BRDF.hlsl"

RaytracingAccelerationStructure g_Scene                     : register(t0);
Texture2D                       g_GlobalTextureData[]       : register(t1, space0);
StructuredBuffer<MeshVertex>    g_GlobalMeshVertexData[]    : register(t1, space1);
StructuredBuffer<uint>          g_GlobalMeshIndexData[]     : register(t1, space2);

StructuredBuffer<MeshInfo>      g_GlobalMeshInfo            : register(t2, space3);
StructuredBuffer<MaterialInfo>  g_GlobalMaterialInfo        : register(t3, space4);
StructuredBuffer<Material>      g_GlobalMaterials           : register(t4, space5);

Texture2D                       g_GBufferHeap[]             : register(t5, space6);

SamplerState                    g_NearestRepeatSampler      : register(s0);
SamplerState                    g_LinearRepeatSampler       : register(s1);

ConstantBuffer<DirectionalLightCB> g_DirectionalLight       : register(b0);
ConstantBuffer<CameraCB>           g_Camera                 : register(b1);
ConstantBuffer<RaytracingDataCB>   g_RaytracingData         : register(b2);

struct PixelShaderOutput
{
    float4 DirectDiffuse    : SV_TARGET0;
    float4 IndirectDiffuse  : SV_TARGET1;
    float4 IndirectSpecular : SV_TARGET2;
};

PixelShaderOutput main(float2 TexCoord : TEXCOORD)
{
    float3 g_SkyColor = float3(0.4f, 0.6f, 0.9f);
    
    PixelShaderOutput OUT;

    GFragment fi = UnpackGBuffer(g_GBufferHeap, g_NearestRepeatSampler, TexCoord);
    
    if (fi.shaderModel < 0.48f || fi.shaderModel > 0.52f)
    {
        OUT.DirectDiffuse = float4(g_SkyColor, 1.f);
        return OUT;
    }
    
    float3 fragPos = GetWorldPosFromDepth(TexCoord, fi.depth, g_Camera.invViewProj); 
    OUT.DirectDiffuse = float4(CalculateDiffuse(g_DirectionalLight.direction.rgb, fi.normal, g_DirectionalLight.color.rgb), 1.f);
    //OUT.DirectDiffuse *= g_GlobalTextureData[5].Sample(g_LinearRepeatSampler, TexCoord);

    RayQuery <RAY_FLAG_CULL_NON_OPAQUE|RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES|RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;
            
    uint ray_flags = 0; // Any this ray requires in addition those above.
    uint ray_instance_mask = 0xffffffff;

    RayDesc ray;
    ray.TMin = 0.0001f;
    ray.TMax = 1e10f;
    ray.Origin = g_Camera.pos;
    GetCameraDirectionFromUV(g_Camera.resolution * TexCoord, g_Camera.resolution, g_Camera.pos, g_Camera.invViewProj, ray.Direction);

    float3 color = float3(1.f, 0.f, 0.f);
    
    float3 radiance = float3(0.f, 0.f, 0.f);
    float3 troughput = float3(1.f, 1.f, 1.f);
    
   
    if (DEBUG_RAYTRACING_FINALCOLOR == g_RaytracingData.debugSettings)
    {     
        SurfaceMaterial gBufferMat;
        gBufferMat.albedo = fi.albedo;
        gBufferMat.ao = fi.ao;
        gBufferMat.emissive = fi.emissive;
        gBufferMat.height = fi.height;
        gBufferMat.metallic = fi.metallic;
        gBufferMat.roughness = fi.roughness;
        gBufferMat.normal = fi.normal;

        float3 V = ray.Direction;
        
        RayDesc shadowRayDesc;
        shadowRayDesc.TMin = 0.01f;
        shadowRayDesc.TMax = 1e10f;
        shadowRayDesc.Origin = fragPos;
        shadowRayDesc.Direction = -g_DirectionalLight.direction.rgb;
        
        query.TraceRayInline(g_Scene, ray_flags, ray_instance_mask, shadowRayDesc);
        query.Proceed();
        
        if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
        {
            
        }
        else
        {
            radiance += troughput * EvaluateBRDF(fi.normal, -g_DirectionalLight.direction.rgb, -V, gBufferMat) * 2.f;
        }
                 
        OUT.DirectDiffuse = float4(radiance, 1.f);
        return OUT;
    }
      
    query.TraceRayInline(g_Scene, ray_flags, ray_instance_mask, ray);
    query.Proceed();
    
    
    if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        HitAttributes hit;
        hit.bFrontFaced = query.CommittedTriangleFrontFace();
        
        if (hit.bFrontFaced == true)
        {
            int instanceIndex = query.CommittedInstanceID();
            hit.primitiveIndex = query.CommittedPrimitiveIndex();
            hit.geometryIndex = query.CommittedGeometryIndex();
        
            hit.barycentrics = query.CommittedTriangleBarycentrics();
            hit.objToWorld = query.CommittedObjectToWorld3x4();
        
            MeshInfo meshInfo = g_GlobalMeshInfo[instanceIndex];
            MaterialInfo materialInfo = g_GlobalMaterialInfo[meshInfo.materialInstanceID];
            MeshVertex hitSurface = GetHitSurface(hit, meshInfo, g_GlobalMeshVertexData, g_GlobalMeshIndexData);
            SurfaceMaterial hitSurfaceMaterial = GetSurfaceMaterial(materialInfo, hitSurface.textureCoordinate, g_LinearRepeatSampler, g_GlobalTextureData);
            
            if (DEBUG_RAYTRACING_ALBEDO == g_RaytracingData.debugSettings)
            {
                color = hitSurfaceMaterial.albedo;
            }
            else if (DEBUG_RAYTRACING_NORMAL == g_RaytracingData.debugSettings)
            {
                color = TangentToWorldNormal(hitSurface.tangent,
                hitSurface.bitangent,
                hitSurface.normal,
                hitSurfaceMaterial.normal,
                hit.objToWorld
                );
            }
            else if (DEBUG_RAYTRACING_PBR == g_RaytracingData.debugSettings)
            {
                color = float3(hitSurfaceMaterial.ao, hitSurfaceMaterial.roughness, hitSurfaceMaterial.metallic);
            }
            else if (DEBUG_RAYTRACING_EMISSIVE == g_RaytracingData.debugSettings)
            {
                color = hitSurfaceMaterial.emissive;
            }
            else if (DEBUG_RAYTRACING_UV == g_RaytracingData.debugSettings)
            {
                color = float3(hitSurface.textureCoordinate, 0.f);
            }
            else if (DEBUG_RAYTRACING_POS == g_RaytracingData.debugSettings)
            {
                color = float3(1.f, 1.f, 1.f);
            }
            else if (DEBUG_RAYTRACING_FINALCOLOR == g_RaytracingData.debugSettings)
            {
                float3 hitNormal = TangentToWorldNormal(hitSurface.tangent,
                hitSurface.bitangent,
                hitSurface.normal,
                hitSurfaceMaterial.normal,
                hit.objToWorld
                );        
                
            }
            
        }
    }
    else
    {
        color = float3(0.f, 0.f, 1.f);
    }
    
    OUT.IndirectDiffuse = float4(color, 1.f);
    
    return OUT;
}