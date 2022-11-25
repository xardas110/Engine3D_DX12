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

RWTexture2D<float4>             g_GlobalUAV[]               : register(u0);
 
static RayQuery<RAY_FLAG_CULL_NON_OPAQUE|RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES|RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;
static uint ray_flags = 0; // Any this ray requires in addition those above.
static uint ray_instance_mask = 0xffffffff;
    
struct PixelShaderOutput
{
    float4 DirectDiffuse    : SV_TARGET0;
    float4 IndirectDiffuse  : SV_TARGET1;
    float4 IndirectSpecular : SV_TARGET2;
    float4 rtDebug          : SV_TARGET3;
};
  
PixelShaderOutput main(float2 TexCoord : TEXCOORD)
{
    PixelShaderOutput OUT;
        
    float3 g_SkyColor = float3(0.4f, 0.6f, 0.9f);
    uint2 pixelCoords = g_Camera.resolution * TexCoord;

    RngStateType rngState = InitRNG(pixelCoords, g_Camera.resolution, g_RaytracingData.frameNumber);
    
    GFragment fi = UnpackGBuffer(g_GBufferHeap, g_NearestRepeatSampler, TexCoord);
    float3 fragPos = GetWorldPosFromDepth(TexCoord, fi.depth, g_Camera.invViewProj);
                    
    if (fi.shaderModel == SM_SKY)
    {
        OUT.DirectDiffuse = float4(g_SkyColor, 1.f);
        OUT.IndirectDiffuse = float4(0.f, 0.f, 0.f, 0.f);
        OUT.IndirectSpecular = float4(0.f, 0.f, 0.f, 0.f);
        return OUT;
    }
        
    RayDesc ray;
    ray.TMin = 0.1f;
    ray.TMax = 1e10f;
    ray.Origin = fragPos;
    ray.Direction = normalize(fragPos.rgb - g_Camera.pos.rgb);

    float3 color = float3(1.f, 0.f, 0.f);   
    float3 directRadiance = float3(0.f, 0.f, 0.f);
    float3 indirectDiffuse = float3(0.f, 0.f, 0.f);
    float3 indirectSpecular = float3(0.f, 0.f, 0.f);       
    float3 troughput = float3(1.f, 1.f, 1.f);
    float3 geometryNormal = float3(0.f, 1.f, 0.f);
        
    SurfaceMaterial currentMat;
    currentMat.albedo = fi.albedo;
    currentMat.ao = fi.ao;
    currentMat.emissive = fi.emissive;
    currentMat.height = fi.height;
    currentMat.metallic = fi.metallic;
    currentMat.roughness = fi.roughness;
    currentMat.normal = fi.normal;
    geometryNormal = fi.normal;
                  
    float3 V = -ray.Direction;
    float3 L = -g_DirectionalLight.direction.rgb;    
                 
    RayDesc shadowRayDesc;             
    shadowRayDesc.TMin = 0.1f;
    shadowRayDesc.TMax = 1e10f;
    shadowRayDesc.Origin = ray.Origin;
    shadowRayDesc.Direction = L;
                
    query.TraceRayInline(g_Scene, ray_flags, ray_instance_mask, shadowRayDesc);
    query.Proceed();
        
    if (query.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
    {
        directRadiance += troughput * EvaluateBRDF(currentMat.normal, L, V, currentMat);
    }
      
    OUT.DirectDiffuse = float4(directRadiance, 1.f);
   
//Direct Lighting END-------------
  
//Indirect Lighting BEGIN----------        
    float firstDiffuseBounceDistance = 0.f;
    float firstSpecularBounceDistance = 0.f;
    for (int i = 0; i < g_RaytracingData.numBounces; i++)
    {                            
        int brdfType;
        if (currentMat.metallic == 1.f && currentMat.roughness == 0.f)
        {
            brdfType = BRDF_TYPE_SPECULAR;
        }
        else
        {
            float brdfProbability = GetBRDFProbability(currentMat, V, currentMat.normal);

            if (Rand(rngState) < brdfProbability)
            {
                brdfType = BRDF_TYPE_SPECULAR;
                troughput /= brdfProbability;
            }
            else
            {
                brdfType = BRDF_TYPE_DIFFUSE;
                troughput /= (1.0f - brdfProbability);
            }
        }

        float3 brdfWeight;
        float2 u = float2(Rand(rngState), Rand(rngState));               
        if (!EvaluateIndirectBRDF(u, currentMat.normal, geometryNormal, V, currentMat, brdfType, ray.Direction, brdfWeight))
        {
            break;
        }
           
        troughput *= brdfWeight;
        V = -ray.Direction;
                
        query.TraceRayInline(g_Scene, ray_flags, ray_instance_mask, ray);
        query.Proceed();
        
        if (query.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
        {                
            if (brdfType == BRDF_TYPE_DIFFUSE)
            {     
                firstDiffuseBounceDistance = 50001.f;
                indirectDiffuse += troughput * g_SkyColor;      
            }
            else
                
            {              
                firstSpecularBounceDistance = 50001.f;
                indirectSpecular += troughput * g_SkyColor;
            }           
            break;
        }
                     
        HitAttributes hit;
        hit.bFrontFaced = query.CommittedTriangleFrontFace();
        int instanceIndex = query.CommittedInstanceID();
        hit.primitiveIndex = query.CommittedPrimitiveIndex();
        hit.geometryIndex = query.CommittedGeometryIndex();     
        hit.barycentrics = query.CommittedTriangleBarycentrics();
        hit.objToWorld = query.CommittedObjectToWorld3x4();
                         
        MeshInfo meshInfo = g_GlobalMeshInfo[instanceIndex];
        MaterialInfo materialInfo = g_GlobalMaterialInfo[meshInfo.materialInstanceID];
                
        MeshVertex hitSurface = GetHitSurface(hit, meshInfo, g_GlobalMeshVertexData, g_GlobalMeshIndexData);
        hitSurface.normal = RotatePoint(meshInfo.objRot, hitSurface.normal);
        hitSurface.normal = normalize(mul(hit.objToWorld, float4(hitSurface.normal, 0.0f)).xyz);
        hitSurface.position = mul(hit.objToWorld, float4(hitSurface.position, 1.f));
        geometryNormal = hitSurface.normal;
                
        if (brdfType == BRDF_TYPE_DIFFUSE)
        {
            if (i == 0)
            {
                firstDiffuseBounceDistance = query.CommittedRayT();
            }
        }
        else
        {
            if (i == 0)
            {
                firstSpecularBounceDistance = query.CommittedRayT();
            }
        }
                        
        bool bMatHasNormal;
        SurfaceMaterial hitSurfaceMaterial = GetSurfaceMaterial(materialInfo, hitSurface.textureCoordinate, bMatHasNormal, g_LinearRepeatSampler, g_GlobalTextureData);
               
        if (bMatHasNormal == true)
        {         
            hitSurfaceMaterial.normal =
            TangentToWorldNormal(hitSurface.tangent,
            hitSurface.bitangent,
            hitSurface.normal,
            hitSurfaceMaterial.normal,
            hit.objToWorld
            );
        }
        else
        {
            hitSurfaceMaterial.normal = hitSurface.normal;    
        }
                  
        shadowRayDesc.TMin = 0.0f;
        shadowRayDesc.Origin = OffsetRay(hitSurface.position, hitSurface.normal);
        shadowRayDesc.Direction = L;
                
        query.TraceRayInline(g_Scene, ray_flags, ray_instance_mask, shadowRayDesc);
        query.Proceed();
        if (query.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
        {
            BRDFData data = PrepareBRDFData(hitSurface.normal, L, V, hitSurfaceMaterial);              
            indirectDiffuse += troughput * (EvaluateDiffuseBRDF(data));
            indirectSpecular += troughput * EvaluateSpecularBRDF(data);
        }
                        
        currentMat = hitSurfaceMaterial;
        ray.TMin = 0.0f;
        ray.Origin = shadowRayDesc.Origin;
    }
    
    float3 finalGather = directRadiance + indirectDiffuse + indirectSpecular;
        
    OUT.IndirectDiffuse = REBLUR_FrontEnd_PackRadianceAndNormHitDist(indirectDiffuse, firstDiffuseBounceDistance);
    OUT.IndirectSpecular = REBLUR_FrontEnd_PackRadianceAndNormHitDist(indirectSpecular, firstSpecularBounceDistance);
    OUT.DirectDiffuse = float4(directRadiance, 1.f);
        
    if (DEBUG_FINAL_COLOR == g_RaytracingData.debugSettings)
    {           
        return OUT;
    }
              
    ray.TMin = 0.f;
    ray.Origin = g_Camera.pos;
    GetCameraDirectionFromUV(pixelCoords, g_Camera.resolution, g_Camera.pos, g_Camera.invViewProj, ray.Direction);
        
    query.TraceRayInline(g_Scene, ray_flags, ray_instance_mask, ray);
    query.Proceed();
                  
    if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        HitAttributes hit;
        hit.bFrontFaced = query.CandidateTriangleFrontFace();
        
        int instanceIndex = query.CandidateInstanceID();
        hit.primitiveIndex = query.CandidatePrimitiveIndex();
        hit.geometryIndex = query.CandidateGeometryIndex();
        
        hit.barycentrics = query.CandidateTriangleBarycentrics();
        hit.objToWorld = query.CandidateObjectToWorld3x4();
        
        MeshInfo meshInfo = g_GlobalMeshInfo[instanceIndex];
        MaterialInfo materialInfo = g_GlobalMaterialInfo[meshInfo.materialInstanceID];
        MeshVertex hitSurface = GetHitSurface(hit, meshInfo, g_GlobalMeshVertexData, g_GlobalMeshIndexData);
        hitSurface.normal = RotatePoint(meshInfo.objRot, hitSurface.normal);
        hitSurface.normal = normalize(mul(hit.objToWorld, float4(hitSurface.normal, 0.0f)).xyz);
        bool bMatHasNormal;
        SurfaceMaterial hitSurfaceMaterial = GetSurfaceMaterial(materialInfo, hitSurface.textureCoordinate, bMatHasNormal, g_LinearRepeatSampler, g_GlobalTextureData);
               
        if (bMatHasNormal == true)
        {
            hitSurfaceMaterial.normal =
            TangentToWorldNormal(hitSurface.tangent,
            hitSurface.bitangent,
            hitSurface.normal,
            hitSurfaceMaterial.normal,
            hit.objToWorld
            );
        }
        else
        {
            hitSurfaceMaterial.normal = hitSurface.normal;
        }
            
        if (DEBUG_RAYTRACED_ALBEDO == g_RaytracingData.debugSettings)
        {
            color = hitSurfaceMaterial.albedo;
        }
        else if (DEBUG_RAYTRACED_NORMAL == g_RaytracingData.debugSettings)
        {
            color = hitSurfaceMaterial.normal.rgb;
        }
        else if (DEBUG_RAYTRACED_AO == g_RaytracingData.debugSettings)
        {
            color = float3(hitSurfaceMaterial.ao, hitSurfaceMaterial.ao, hitSurfaceMaterial.ao);
        }
        else if (DEBUG_RAYTRACED_ROUGHNESS == g_RaytracingData.debugSettings)
        {
            color = float3(hitSurfaceMaterial.roughness, hitSurfaceMaterial.roughness, hitSurfaceMaterial.roughness);
        }
        else if (DEBUG_RAYTRACED_METALLIC == g_RaytracingData.debugSettings)
        {
            color = float3(hitSurfaceMaterial.metallic, hitSurfaceMaterial.metallic, hitSurfaceMaterial.metallic);
        }
        else if (DEBUG_RAYTRACED_HEIGHT == g_RaytracingData.debugSettings)
        {
            color = float3(hitSurfaceMaterial.height, hitSurfaceMaterial.height, hitSurfaceMaterial.height);
        }
        else if (DEBUG_RAYTRACED_EMISSIVE == g_RaytracingData.debugSettings)
        {
            color = hitSurfaceMaterial.emissive;
        }            
    }
    else
    {
        color = g_SkyColor;
    }
        
    OUT.rtDebug = float4(color, 1.f);
    
    return OUT;
};