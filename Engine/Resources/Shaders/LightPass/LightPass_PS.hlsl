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
SamplerState                    g_LinearClampSampler        : register(s2);

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
  
void TestRT(RayDesc ray, inout float3 debugColor);

bool TraceTranslucency(RayInfo ray, inout float3 troughput, inout float3 radiance)
{      
    RayQuery < RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES > query;
    query.TraceRayInline(g_Scene, ray.flags, ray.instanceMask, ray.desc);
    
    while (query.Proceed())
    {
        HitAttributes hit;
        hit.bFrontFaced = query.CandidateTriangleFrontFace();
        hit.instanceIndex = query.CandidateInstanceID();
        hit.primitiveIndex = query.CandidatePrimitiveIndex();
        hit.geometryIndex = query.CandidateGeometryIndex();
        hit.barycentrics = query.CandidateTriangleBarycentrics();
        hit.objToWorld = query.CandidateObjectToWorld3x4();
        hit.minT = query.CandidateTriangleRayT();

        MeshInfo meshInfo = g_GlobalMeshInfo[hit.instanceIndex];
        MaterialInfo materialInfo = g_GlobalMaterialInfo[meshInfo.materialInstanceID];
        
           
        MeshVertex hitSurface = GetHitSurface(hit, meshInfo, g_GlobalMeshVertexData, g_GlobalMeshIndexData);
        SurfaceMaterial hitSurfaceMaterial = GetSurfaceMaterial(materialInfo, hitSurface, hit.objToWorld, meshInfo.objRot, g_LinearClampSampler, g_GlobalTextureData);
           
        float3 rayHit = mul(hit.objToWorld, float4(hitSurface.position, 1.f));
        float3 geometryNormal = RotatePoint(meshInfo.objRot, hitSurface.normal);
        
        float3 V = -ray.desc.Direction;
        float3 L = -g_DirectionalLight.direction.rgb;
        float3 X = rayHit;
         
        if (dot(geometryNormal, V) < 0.0f)
            geometryNormal = -geometryNormal;
        if (dot(geometryNormal, hitSurfaceMaterial.normal) < 0.0f)
            hitSurfaceMaterial.normal = -hitSurfaceMaterial.normal;
             
        float3 rayHitOffset = OffsetRay(rayHit, geometryNormal);
                
        ApplyMaterial(materialInfo, hitSurfaceMaterial, g_GlobalMaterials);
        hitSurfaceMaterial.albedo = pow(hitSurfaceMaterial.albedo, 2.2f);
    
        RayInfo shadowRayInfo;
        RayDesc shadowRayDesc;
    
        shadowRayDesc.TMin = 0.f;
        shadowRayDesc.TMax = 1e10f;
        shadowRayDesc.Origin = X;
        shadowRayDesc.Direction = L;
    
        shadowRayInfo.desc = shadowRayDesc;
        shadowRayInfo.flags = 0;
        shadowRayInfo.instanceMask = INSTANCE_OPAQUE;
    
      //  if (TraceVisibility(shadowRayInfo, g_Scene))
       // {
            float3 color = EvaluateBRDF(hitSurfaceMaterial.normal, L, V, hitSurfaceMaterial) * g_DirectionalLight.color.w * g_DirectionalLight.color.rgb;
            radiance = lerp(radiance, hitSurfaceMaterial.albedo, hitSurfaceMaterial.opacity);
            
     //   }        
    };
    
    return query.CommittedStatus() != COMMITTED_NOTHING;
}

// Generates a primary ray for pixel given in NDC space using pinhole camera
RayDesc GeneratePinholeCameraRay(float2 pixel, float depth)
{
    float4x4 view = transpose(g_Camera.invView);
    
	// Setup the ray
    RayDesc ray;
    ray.Origin = view[3].xyz;
    ray.TMin = 0.1f;
    ray.TMax = depth;
    
	// Extract the aspect ratio and field of view from the projection matrix
    float aspect = g_Camera.proj[1][1] / g_Camera.proj[0][0];
    float tanHalfFovY = 1.0f / g_Camera.proj[1][1];

	// Compute the ray direction for this pixel
    ray.Direction = normalize(
		(pixel.x * view[0].xyz * tanHalfFovY * aspect) -
		(pixel.y * view[1].xyz * tanHalfFovY) +
			view[2].xyz);

    return ray;
}


PixelShaderOutput main(float2 TexCoord : TEXCOORD)
{
    PixelShaderOutput OUT;
        
    uint2 pixelCoords = g_Camera.resolution * TexCoord;

    RngStateType rngState = InitRNG(pixelCoords, g_Camera.resolution, g_RaytracingData.frameNumber);
    
    GFragment fi = UnpackGBuffer(g_GBufferHeap, g_NearestRepeatSampler, TexCoord);    
    fi.albedo = pow(fi.albedo, 2.2f); 
    
    float3 pixelWS = GetWorldPosFromDepth(TexCoord, fi.depth, g_Camera.invViewProj);
                 
    if (fi.shaderModel == SM_SKY)
    {
        OUT.DirectDiffuse = float4(0.f, 0.f, 0.f, 0.f);
        OUT.IndirectDiffuse = float4(0.f, 0.f, 0.f, 0.f);
        OUT.IndirectSpecular = float4(0.f, 0.f, 0.f, 0.f);
        return OUT;
    }
     
    float3 directRadiance = float3(0.f, 0.f, 0.f);
    float3 indirectRadiance = float3(0.f, 0.f, 0.f);
    
    float3 indirectDiffuse = float3(0.f, 0.f, 0.f);
    float3 indirectSpecular = float3(0.f, 0.f, 0.f);
    
    float3 troughput = float3(1.f, 1.f, 1.f);
    
    float3 geometryNormal = fi.geometryNormal;
          
    RayDesc ray;
    ray.TMin = 0.f;
    ray.TMax = 1e10f;
    ray.Origin = OffsetRay(pixelWS, geometryNormal) + geometryNormal * 0.1f;
    ray.Direction = normalize(pixelWS.rgb - g_Camera.pos.rgb);

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
             
    BRDFData gBufferBRDF = PrepareBRDFData(currentMat.normal, L, V, currentMat);
    
    RayInfo shadowRayInfo;   
    RayDesc shadowRayDesc;
    
    shadowRayDesc.TMin = 0.f;
    shadowRayDesc.TMax = 1e10f;
    shadowRayDesc.Origin = ray.Origin;
    shadowRayDesc.Direction = L;
    
    shadowRayInfo.desc = shadowRayDesc;
    shadowRayInfo.flags = 0;
    shadowRayInfo.instanceMask = INSTANCE_OPAQUE;
    
    if (TraceVisibility(shadowRayInfo, g_Scene))
    {        
        directRadiance += troughput * EvaluateBRDF(currentMat.normal, L, V, currentMat) * g_DirectionalLight.color.w * g_DirectionalLight.color.rgb;
    }
      
//Direct Lighting END-------------
//Indirect Lighting BEGIN----------        
    float firstDiffuseBounceDistance = 0.f;
    float firstSpecularBounceDistance = 0.f;
    float viewZ = g_GBufferHeap[GBUFFER_LINEAR_DEPTH].Sample(g_NearestRepeatSampler, TexCoord).r;
          
    float diffuseWeight = 0.f;
    float specWeight = 0.f;
    
    for (int i = 0; i < g_RaytracingData.numBounces; i++)
    {       
        int brdfType;
        if (currentMat.metallic == 1.f && currentMat.roughness == 0.f)
        {
            brdfType = BRDF_TYPE_SPECULAR; 
            
            if (i == 0)
                specWeight = 1.f;
        }
        else
        {
            float brdfProbability = GetBRDFProbability(currentMat, V, currentMat.normal);

            if (Rand(rngState) < brdfProbability)
            {
                brdfType = BRDF_TYPE_SPECULAR;
                troughput /= brdfProbability;
                
                if (i == 0)
                    specWeight = 1.f;
            }
            else
            {
                brdfType = BRDF_TYPE_DIFFUSE;
                troughput /= (1.0f - brdfProbability);
                
                if (i == 0)
                    diffuseWeight = 1.f;
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
                     
        RayInfo opaqueRay;
        opaqueRay.desc = ray;
        opaqueRay.flags = 0;
        opaqueRay.instanceMask = INSTANCE_OPAQUE;
           
        HitAttributes hit;   
        if (!CastRay(opaqueRay, g_Scene, hit))
        {                
            indirectRadiance += troughput * SampleSky(ray.Direction, g_Cubemap, g_LinearRepeatSampler).rgb;          
            indirectDiffuse += indirectRadiance * diffuseWeight;
            indirectSpecular += indirectRadiance * specWeight;
            
            break;
        }
                     
        MeshInfo meshInfo = g_GlobalMeshInfo[hit.instanceIndex];
        MaterialInfo materialInfo = g_GlobalMaterialInfo[meshInfo.materialInstanceID];
                
        MeshVertex hitSurface = GetHitSurface(hit, meshInfo, g_GlobalMeshVertexData, g_GlobalMeshIndexData);           
        SurfaceMaterial hitSurfaceMaterial = GetSurfaceMaterial(materialInfo, hitSurface, hit.objToWorld, meshInfo.objRot, g_LinearRepeatSampler, g_GlobalTextureData);
        
        float3 rayHit = mul(hit.objToWorld, float4(hitSurface.position, 1.f));
        geometryNormal = RotatePoint(meshInfo.objRot, hitSurface.normal);
        
        float3 rayHitOffset = OffsetRay(rayHit, geometryNormal);
                
        ApplyMaterial(materialInfo, hitSurfaceMaterial, g_GlobalMaterials);           
        hitSurfaceMaterial.albedo = pow(hitSurfaceMaterial.albedo, 2.2f);
          
        if (i == 0)
        {
            float w = NRD_GetSampleWeight(indirectRadiance);           
            firstDiffuseBounceDistance = REBLUR_FrontEnd_GetNormHitDist(hit.minT, viewZ, g_RaytracingData.hitParams, fi.roughness) * w * diffuseWeight;
            firstSpecularBounceDistance = REBLUR_FrontEnd_GetNormHitDist(hit.minT, viewZ, g_RaytracingData.hitParams, fi.roughness) * w * specWeight;
        }
               
        shadowRayDesc.TMin = 0.f;
        shadowRayDesc.TMax = 10000.f;
        shadowRayDesc.Origin = rayHitOffset;
        shadowRayDesc.Direction = L;
        
        shadowRayInfo.desc = shadowRayDesc;

        if (TraceVisibility(shadowRayInfo, g_Scene))
        {
            indirectRadiance += troughput * EvaluateBRDF(hitSurfaceMaterial.normal, L, V, hitSurfaceMaterial) * g_DirectionalLight.color.w * g_DirectionalLight.color.rgb;
        }
         
        indirectDiffuse += indirectRadiance * diffuseWeight;
        indirectSpecular += indirectRadiance * specWeight;
        
        currentMat = hitSurfaceMaterial;
        ray.Origin = rayHitOffset;
    }
    
    float3 fenv = ApproxSpecularIntegralGGX(gBufferBRDF.specularF0, gBufferBRDF.alpha, gBufferBRDF.NdotV);
    float3 diffDemod = (1.f - fenv) * gBufferBRDF.diffuseReflectance;
    float3 specDemod = fenv;
    
    indirectDiffuse.rgb /= (diffDemod * 0.99f + 0.01f);
    indirectSpecular.rgb /= (specDemod * 0.99f + 0.01f);
    
    //Translucency 
    float3 translucentTroughput = float3(1.f, 1.f, 1.f);

    ray.TMin = g_Camera.zNear;
    ray.TMax = viewZ;
    ray.Origin = g_Camera.pos;    
    GetCameraDirectionFromUV(pixelCoords, g_Camera.resolution, g_Camera.pos, g_Camera.invViewProj, ray.Direction);

    RayInfo tranclucentRay;
    tranclucentRay.desc = GeneratePinholeCameraRay(TexCoord * 2.f - 1.f, viewZ);
    tranclucentRay.flags = 0;
    tranclucentRay.instanceMask = INSTANCE_TRANSLUCENT | INSTANCE_OPAQUE;
    
    bool bHit = TraceTranslucency(tranclucentRay, translucentTroughput, directRadiance);
    
    OUT.DirectDiffuse = float4(directRadiance, 1.f);
    OUT.IndirectDiffuse = REBLUR_FrontEnd_PackRadianceAndNormHitDist(indirectDiffuse, firstDiffuseBounceDistance);
    OUT.IndirectSpecular = REBLUR_FrontEnd_PackRadianceAndNormHitDist(indirectSpecular, firstSpecularBounceDistance);
    
    if (DEBUG_FINAL_COLOR == g_RaytracingData.debugSettings)
    {
        return OUT;
    }
    
    ray.TMax = 10000.f;
    
    float3 rtDebugColor = float3(0.f, 0.f, 0.f);
    TestRT(ray, rtDebugColor);
    
    if (DEBUG_RAYTRACED_HIT_T == g_RaytracingData.debugSettings)
    {
        rtDebugColor = float3(firstDiffuseBounceDistance, firstDiffuseBounceDistance, firstDiffuseBounceDistance);
    } 
  
    OUT.rtDebug = float4(rtDebugColor, 1.f);
    
    return OUT;
};

void TestRT(RayDesc ray, inout float3 debugColor)
{
    RayQuery < RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES > query;
    query.TraceRayInline(g_Scene, 0, INSTANCE_OPAQUE, ray);
    
    while (query.Proceed())
    {
    }
                  
    if (query.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
        return;
    
    HitAttributes hit;
    hit.bFrontFaced = query.CandidateTriangleFrontFace();
        
    hit.instanceIndex = query.CandidateInstanceID();
    hit.primitiveIndex = query.CandidatePrimitiveIndex();
    hit.geometryIndex = query.CandidateGeometryIndex();
        
    hit.barycentrics = query.CandidateTriangleBarycentrics();
    hit.objToWorld = query.CandidateObjectToWorld3x4();
        
    MeshInfo meshInfo = g_GlobalMeshInfo[hit.instanceIndex];
    MaterialInfo materialInfo = g_GlobalMaterialInfo[meshInfo.materialInstanceID];
    MeshVertex hitSurface = GetHitSurface(hit, meshInfo, g_GlobalMeshVertexData, g_GlobalMeshIndexData);

    SurfaceMaterial hitSurfaceMaterial = GetSurfaceMaterial(materialInfo, hitSurface, hit.objToWorld, meshInfo.objRot, g_LinearRepeatSampler, g_GlobalTextureData);
    ApplyMaterial(materialInfo, hitSurfaceMaterial, g_GlobalMaterials);
            
    hitSurface.normal = RotatePoint(meshInfo.objRot, hitSurface.normal);
        
    if (DEBUG_RAYTRACED_ALBEDO == g_RaytracingData.debugSettings)
    {
        debugColor = hitSurfaceMaterial.albedo;
    }
    else if (DEBUG_RAYTRACED_NORMAL == g_RaytracingData.debugSettings)
    {
        debugColor = hitSurfaceMaterial.normal.rgb;
    }
    else if (DEBUG_RAYTRACED_AO == g_RaytracingData.debugSettings)
    {
        debugColor = float3(hitSurfaceMaterial.ao, hitSurfaceMaterial.ao, hitSurfaceMaterial.ao);
    }
    else if (DEBUG_RAYTRACED_ROUGHNESS == g_RaytracingData.debugSettings)
    {
        debugColor = float3(hitSurfaceMaterial.roughness, hitSurfaceMaterial.roughness, hitSurfaceMaterial.roughness);
    }
    else if (DEBUG_RAYTRACED_METALLIC == g_RaytracingData.debugSettings)
    {
        debugColor = float3(hitSurfaceMaterial.metallic, hitSurfaceMaterial.metallic, hitSurfaceMaterial.metallic);
    }
    else if (DEBUG_RAYTRACED_HEIGHT == g_RaytracingData.debugSettings)
    {
        debugColor = float3(hitSurfaceMaterial.height, hitSurfaceMaterial.height, hitSurfaceMaterial.height);
    }
    else if (DEBUG_RAYTRACED_EMISSIVE == g_RaytracingData.debugSettings)
    {
        debugColor = hitSurfaceMaterial.emissive;
    }
}
