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

TextureCube<float4>             g_Cubemap                   : register(t7, space8);
    
SamplerState                    g_NearestRepeatSampler      : register(s0);
SamplerState                    g_LinearRepeatSampler       : register(s1);

ConstantBuffer<DirectionalLightCB> g_DirectionalLight       : register(b0);
ConstantBuffer<CameraCB>           g_Camera                 : register(b1);
ConstantBuffer<RaytracingDataCB>   g_RaytracingData         : register(b2);

RWTexture2D<float4>             g_GlobalUAV[]               : register(u0);
  
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
        
    uint2 pixelCoords = g_Camera.resolution * TexCoord;

    RngStateType rngState = InitRNG(pixelCoords, g_Camera.resolution, g_RaytracingData.frameNumber);
    
    GFragment fi = UnpackGBuffer(g_GBufferHeap, g_NearestRepeatSampler, TexCoord);    
    fi.albedo = pow(fi.albedo, 2.2f);
        
    float3 fragPos = GetWorldPosFromDepth(TexCoord, fi.depth, g_Camera.invViewProj) + (fi.normal * 0.1f);
                    
    if (fi.shaderModel == SM_SKY)
    {
        OUT.DirectDiffuse = float4(0.f, 0.f, 0.f, 0.f);
        OUT.IndirectDiffuse = float4(0.f, 0.f, 0.f, 0.f);
        OUT.IndirectSpecular = float4(0.f, 0.f, 0.f, 0.f);
        return OUT;
    }
    
    RayQuery < RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES > query;
    RayQuery < RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > shadowQuery;

    uint ray_flags = 0; // Any this ray requires in addition those above.
    uint ray_instance_mask = 0xffffffff;
        
    RayDesc ray;
    ray.TMin = 0.f;
    ray.TMax = 1e10f;
    ray.Origin = OffsetRay(fragPos, fi.geometryNormal);
    ray.Direction = normalize(fragPos.rgb - g_Camera.pos.rgb);

    float3 color = float3(1.f, 0.f, 0.f);   
    float3 directRadiance = float3(0.f, 0.f, 0.f);
    float3 indirectDiffuse = float3(0.f, 0.f, 0.f);
    float3 indirectSpecular = float3(0.f, 0.f, 0.f);       
    float3 troughput = float3(1.f, 1.f, 1.f);
    float3 geometryNormal = fi.geometryNormal;
        
    SurfaceMaterial currentMat;
    currentMat.albedo = fi.albedo;
    currentMat.ao = fi.ao;
    currentMat.emissive = fi.emissive;
    currentMat.height = fi.height;
    currentMat.metallic = fi.metallic;
    currentMat.roughness = fi.roughness;
    currentMat.normal = fi.normal;
       
    float3 V = -ray.Direction;
    float3 L = -g_DirectionalLight.direction.rgb;    
                 
    RayDesc shadowRayDesc;             
    shadowRayDesc.TMin = 0.f;
    shadowRayDesc.TMax = 1e10f;
    shadowRayDesc.Origin = ray.Origin;
    shadowRayDesc.Direction = L;
                
    shadowQuery.TraceRayInline(g_Scene, ray_flags, ray_instance_mask, shadowRayDesc);
    shadowQuery.Proceed();
        
    BRDFData gBufferBRDF = PrepareBRDFData(currentMat.normal, L, V, currentMat);
      
    //directRadiance += fi.emissive;
       
    if (shadowQuery.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
    {        
        float3 diffuse = troughput * EvaluateDiffuseBRDF(gBufferBRDF);
        float3 specular = troughput * EvaluateSpecularBRDF(gBufferBRDF);
            
        directRadiance += (diffuse + specular) * g_DirectionalLight.color.w  * g_DirectionalLight.color.rgb;
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
  
        if (dot(geometryNormal, V) < 0.0f)
            geometryNormal = -geometryNormal;
        if (dot(geometryNormal, currentMat.normal) < 0.0f)
            currentMat.normal = -currentMat.normal;
        
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
                if (i == 0)
                    firstDiffuseBounceDistance = 50000.f;
                
                indirectDiffuse += troughput * SampleSky(ray.Direction, g_Cubemap, g_LinearRepeatSampler).rgb;      
            }
            else               
            {       
                if (i == 0)
                    firstSpecularBounceDistance = 50000.f;
                
                indirectSpecular += troughput * SampleSky(ray.Direction, g_Cubemap, g_LinearRepeatSampler).rgb;
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
        //hitSurface.normal = normalize(mul(hit.objToWorld, float4(hitSurface.normal, 0.0f)).xyz);
        hitSurface.position = mul(hit.objToWorld, float4(hitSurface.position, 1.f));
        geometryNormal = hitSurface.normal;
                       
        bool bMatHasNormal;
        SurfaceMaterial hitSurfaceMaterial = GetSurfaceMaterial(materialInfo, hitSurface.textureCoordinate, bMatHasNormal, g_LinearRepeatSampler, g_GlobalTextureData);
        ApplyMaterial(materialInfo, hitSurfaceMaterial, g_GlobalMaterials);
            
        hitSurfaceMaterial.albedo = pow(hitSurfaceMaterial.albedo, 2.2f);
            
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

        if (brdfType == BRDF_TYPE_DIFFUSE)
        {
            if (i == 0)
            {
                firstDiffuseBounceDistance = query.CommittedRayT();
            }
                
            indirectDiffuse += troughput * hitSurfaceMaterial.emissive;
               
        }       
        else
        {
            if (i == 0)
            {
                firstSpecularBounceDistance = query.CommittedRayT();
            }
                
            indirectSpecular += troughput * hitSurfaceMaterial.emissive;
        }
                        
        shadowRayDesc.TMin = 0.0f;
        shadowRayDesc.Origin = OffsetRay(hitSurface.position, geometryNormal);
        shadowRayDesc.Direction = L;
                
        shadowQuery.TraceRayInline(g_Scene, ray_flags, ray_instance_mask, shadowRayDesc);
        shadowQuery.Proceed();
        
        if (shadowQuery.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
        {
            BRDFData data = PrepareBRDFData(hitSurfaceMaterial.normal, L, V, hitSurfaceMaterial);
            indirectDiffuse += troughput * (EvaluateDiffuseBRDF(data)) * g_DirectionalLight.color.w * g_DirectionalLight.color.rgb;
            indirectSpecular += troughput * (EvaluateSpecularBRDF(data)) * g_DirectionalLight.color.w * g_DirectionalLight.color.rgb;
        }
                        
        currentMat = hitSurfaceMaterial;
        ray.TMin = 0.0f;
        ray.Origin = shadowRayDesc.Origin;
   
    }
    
    float3 fenv = ApproxSpecularIntegralGGX(gBufferBRDF.specularF0, gBufferBRDF.alpha, gBufferBRDF.NdotV);
    float3 diffDemod = (1.f - fenv) * gBufferBRDF.diffuseReflectance;
    float3 specDemod = fenv;
    
    indirectDiffuse.rgb /= (diffDemod * 0.99f + 0.01f);
    indirectSpecular.rgb /= (specDemod * 0.99f + 0.01f);
        
    OUT.IndirectDiffuse = REBLUR_FrontEnd_PackRadianceAndNormHitDist(indirectDiffuse, firstDiffuseBounceDistance);
    OUT.IndirectSpecular = REBLUR_FrontEnd_PackRadianceAndNormHitDist(indirectSpecular, firstSpecularBounceDistance);
    OUT.DirectDiffuse = float4(directRadiance, 1.f);
        
    if (DEBUG_LIGHTBUFFER_INDIRECT_DIFFUSE == g_RaytracingData.debugSettings)
    {
        OUT.IndirectDiffuse = float4(indirectDiffuse, 1.f);
    }
    else if (DEBUG_LIGHTBUFFER_INDIRECT_SPECULAR == g_RaytracingData.debugSettings)
    {
        OUT.IndirectSpecular = float4(indirectSpecular, 1.f);
    }
        
    if (DEBUG_FINAL_COLOR == g_RaytracingData.debugSettings)
    {           
        return OUT;
    }
              
    ray.TMin = 0.0f;
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
        //hitSurface.normal = normalize(mul(hit.objToWorld, float4(hitSurface.normal, 0.0f)).xyz);
            
        bool bMatHasNormal;
        SurfaceMaterial hitSurfaceMaterial = GetSurfaceMaterial(materialInfo, hitSurface.textureCoordinate, bMatHasNormal, g_LinearRepeatSampler, g_GlobalTextureData);
        ApplyMaterial(materialInfo, hitSurfaceMaterial, g_GlobalMaterials);
            
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
        else if (DEBUG_RAYTRACED_HIT_T == g_RaytracingData.debugSettings)
        {
            OUT.DirectDiffuse = float4(query.CommittedRayT(), query.CommittedRayT(), query.CommittedRayT(), 1.f);
        }
    }
    else
    {
        color = float4(1.f, 0.f, 0.f, 1.f);
    }
        
    OUT.rtDebug = float4(color, 1.f);
    
    return OUT;
};