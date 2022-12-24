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
  

#define DIRECT_LIGHT_FLAG_SKIP_SKY (1 << 0)
#define DIRECT_LIGHT_FLAG_SKIP_OPAQUE (1 << 1)
#define DIRECT_LIGHT_FLAG_SKIP_CUTOFF (1 << 2)
#define DIRECT_LIGHT_FLAG_SAVE_OPACITY (1 << 3)

static float reflectiveAmbient = 0.005f;

struct PixelShaderOutput
{
    float4 DirectLight      : SV_TARGET0;
    float4 IndirectDiffuse  : SV_TARGET1;
    float4 IndirectSpecular : SV_TARGET2;
    float4 rtDebug          : SV_TARGET3;
    float4 TransparentColor : SV_TARGET4;
};
 
struct TraceResult
{
    HitAttributes hit;
    SurfaceMaterial mat;
    MeshVertex vert;
    float3 hitPos;
    float3 geoNormal;
    int depth;
};

void TestRT(RayDesc ray, inout float3 debugColor);

bool TestOpacity(in HitAttributes hit, inout float opacity, float cutOff = 0.5f)
{
    MeshInfo meshInfo = g_GlobalMeshInfo[hit.instanceIndex];
    MaterialInfo materialInfo = g_GlobalMaterialInfo[meshInfo.materialInstanceID];
            
    MeshVertex hitSurface = GetHitSurface(hit, meshInfo, g_GlobalMeshVertexData, g_GlobalMeshIndexData);
    SurfaceMaterial hitSurfaceMaterial = GetSurfaceMaterial(materialInfo, hitSurface, hit.objToWorld, meshInfo.objRot, g_LinearRepeatSampler, g_GlobalTextureData);             
    ApplyMaterial(materialInfo, hitSurfaceMaterial, g_GlobalMaterials);

    opacity = hitSurfaceMaterial.opacity;
    
    if (hitSurfaceMaterial.opacity < cutOff)
        return true;
    
    return false;
}

bool CastRay(RayInfo ray, RaytracingAccelerationStructure scene, inout HitAttributes hit)
{
    RayQuery < RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES > query;
    
    query.TraceRayInline(scene, ray.flags, ray.instanceMask, ray.desc);
    
    while (query.Proceed())
    {
        hit.bFrontFaced = query.CandidateTriangleFrontFace();
        hit.instanceIndex = query.CandidateInstanceID();
        hit.primitiveIndex = query.CandidatePrimitiveIndex();
        hit.geometryIndex = query.CandidateGeometryIndex();
        hit.barycentrics = query.CandidateTriangleBarycentrics();
        hit.objToWorld = query.CandidateObjectToWorld3x4();
        hit.minT = query.CandidateTriangleRayT();
        
        float opacity;
        if (!TestOpacity(hit, opacity))      
            query.CommitNonOpaqueTriangleHit();
    };
  
    hit.bFrontFaced = query.CommittedTriangleFrontFace();
    hit.instanceIndex = query.CommittedInstanceID();
    hit.primitiveIndex = query.CommittedPrimitiveIndex();
    hit.geometryIndex = query.CommittedGeometryIndex();
    hit.barycentrics = query.CommittedTriangleBarycentrics();
    hit.objToWorld = query.CommittedObjectToWorld3x4();
    hit.minT = query.CommittedRayT();
    
    return query.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
}

float TraceVisibility(RayInfo ray, RaytracingAccelerationStructure scene)
{
    RayQuery < RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > shadowQuery;
    shadowQuery.TraceRayInline(scene, ray.flags, ray.instanceMask, ray.desc);
    
    float visibility = 1.f;
    HitAttributes hit;
        
    while (shadowQuery.Proceed())
    {
        hit.bFrontFaced = shadowQuery.CandidateTriangleFrontFace();
        hit.instanceIndex = shadowQuery.CandidateInstanceID();
        hit.primitiveIndex = shadowQuery.CandidatePrimitiveIndex();
        hit.geometryIndex = shadowQuery.CandidateGeometryIndex();
        hit.barycentrics = shadowQuery.CandidateTriangleBarycentrics();
        hit.objToWorld = shadowQuery.CandidateObjectToWorld3x4();
        hit.minT = shadowQuery.CandidateTriangleRayT();
        
        float opacity;
        if (!TestOpacity(hit, opacity, 1.f))      
            shadowQuery.CommitNonOpaqueTriangleHit();

        visibility *= 1.f - opacity;       
    }
    
    if (shadowQuery.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
        return visibility;
    
    hit.bFrontFaced = shadowQuery.CommittedTriangleFrontFace();
    hit.instanceIndex = shadowQuery.CommittedInstanceID();
    hit.primitiveIndex = shadowQuery.CommittedPrimitiveIndex();
    hit.geometryIndex = shadowQuery.CommittedGeometryIndex();
    hit.barycentrics = shadowQuery.CommittedTriangleBarycentrics();
    hit.objToWorld = shadowQuery.CommittedObjectToWorld3x4();
    hit.minT = shadowQuery.CommittedRayT();
    
    float opacity;
    TestOpacity(hit, opacity, 1.f);
    visibility *= 1.f - opacity;
    
    return visibility;
}

//Returns true on refraction
bool TraceDirectLight(RayInfo ray, RngStateType rng, float ambientFactor, uint flags, inout float3 troughput, inout float4 radiance, inout TraceResult refractInfo)
{  
    HitAttributes hit;
    if (!CastRay(ray, g_Scene, hit))
    {   
        if (flags & DIRECT_LIGHT_FLAG_SKIP_SKY)
            return false;
        
        float3 color = SampleSky(ray.desc.Direction, g_Cubemap, g_LinearRepeatSampler).rgb;
        radiance.rgb += color * troughput;
        
        return false;
    }
           
    MeshInfo meshInfo = g_GlobalMeshInfo[hit.instanceIndex];
    MaterialInfo materialInfo = g_GlobalMaterialInfo[meshInfo.materialInstanceID];
          
    if (flags & DIRECT_LIGHT_FLAG_SKIP_OPAQUE && materialInfo.flags & INSTANCE_OPAQUE)
        return false;
        
    if (flags & DIRECT_LIGHT_FLAG_SKIP_CUTOFF & INSTANCE_ALPHA_CUTOFF)
        return false;
     
    MeshVertex hitSurface = GetHitSurface(hit, meshInfo, g_GlobalMeshVertexData, g_GlobalMeshIndexData);
    SurfaceMaterial hitSurfaceMaterial = GetSurfaceMaterial(materialInfo, hitSurface, hit.objToWorld, meshInfo.objRot, g_LinearRepeatSampler, g_GlobalTextureData);
              
    ApplyMaterial(materialInfo, hitSurfaceMaterial, g_GlobalMaterials);
    hitSurfaceMaterial.albedo = pow(hitSurfaceMaterial.albedo, 2.2f);
   
    if (refractInfo.depth == 0)
        radiance.a = hitSurfaceMaterial.opacity;
      
    float3 rayHit = mul(hit.objToWorld, float4(hitSurface.position, 1.f));
    float3 geometryNormal = RotatePoint(meshInfo.objRot, hitSurface.normal);
        
    float3 V = normalize(-ray.desc.Direction);
        
    if (dot(geometryNormal, V) < 0.0f)
        geometryNormal = -geometryNormal;
    if (dot(geometryNormal, hitSurfaceMaterial.normal) < 0.0f)
        hitSurfaceMaterial.normal = -hitSurfaceMaterial.normal;
                       
    float3 L = -g_DirectionalLight.direction.rgb;
    float3 X = rayHit;
              
    float3 rayHitOffset = OffsetRay(X, geometryNormal);

    refractInfo.mat = hitSurfaceMaterial;
    refractInfo.hit = hit;
    refractInfo.geoNormal = geometryNormal;
    refractInfo.vert = hitSurface;
    refractInfo.hitPos = rayHitOffset;
                       
    ray.desc.TMin = 0.01f;
    ray.desc.Origin = rayHitOffset;

    RayInfo shadowRayInfo;
    RayDesc shadowRayDesc;
    
    shadowRayDesc.TMin = 0.f;
    shadowRayDesc.TMax = 1e10f;
    shadowRayDesc.Origin = ray.desc.Origin;
    shadowRayDesc.Direction = L;
    
    shadowRayInfo.desc = shadowRayDesc;
    shadowRayInfo.flags = 0;
    shadowRayInfo.instanceMask = INSTANCE_OPAQUE | INSTANCE_TRANSLUCENT;
        
    float shadowFactor = TraceVisibility(shadowRayInfo, g_Scene);

    float3 directLight = EvaluateBRDF(hitSurfaceMaterial.normal, L, V, hitSurfaceMaterial) * g_DirectionalLight.color.w * g_DirectionalLight.color.rgb * shadowFactor;
    float3 ambientLight = hitSurfaceMaterial.albedo * ambientFactor * g_DirectionalLight.color.w * g_DirectionalLight.color.rgb;
        
    radiance.rgb += troughput * (directLight + ambientLight + hitSurfaceMaterial.emissive);
            
    return true;
}

/*
bool TranslucentPass(RayInfo ray, RngStateType rng, inout float3 troughput, inout float4 radiance)
{   
    RefractionInfo refractInfo;
    refractInfo.depth = 0;
    bool bResult = TraceTranslucency(ray, rng, troughput, radiance, refractInfo);
    
    if (!bResult)
        return bResult;

    refractInfo.depth = 1;
    
    float3 currentDir = normalize(ray.desc.Direction);
         
    if (refractInfo.mat.roughness < 0.1f)
    {       
        float3 brdfWeight;
        float2 u = float2(Rand(rng), Rand(rng));
        if (EvaluateIndirectBRDF(u, refractInfo.mat.normal, refractInfo.geoNormal, -currentDir, refractInfo.mat, BRDF_TYPE_SPECULAR, ray.desc.Direction, brdfWeight))
        {
            TraceTranslucency(ray, rng, brdfWeight, radiance, refractInfo);
        }       
    }

    return bResult;
}
*/

// Generates a primary ray for pixel given in NDC space using pinhole camera
RayDesc GeneratePinholeCameraRay(float2 pixel)
{
    float4x4 view = transpose(g_Camera.invView);
    
	// Setup the ray
    RayDesc ray;
    ray.Origin = view[3].xyz;
    ray.TMin = 0.0f;
    ray.TMax = 10000.f;
    
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

bool DirectLightGBuffer(in float2 texCoords, RngStateType rng, inout float4 radiance)
{
    GFragment fi = UnpackGBuffer(g_GBufferHeap, g_NearestRepeatSampler, texCoords);
    fi.albedo = pow(fi.albedo, 2.2f);
      
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
        radiance.rgb = SampleSky(-V, g_Cubemap, g_LinearClampSampler);
        return false;
    }
     
    RayInfo shadowRayInfo;
    shadowRayInfo.desc.TMin = 0.00f;
    shadowRayInfo.desc.TMax = 10000.f;
    shadowRayInfo.desc.Origin = X;
    shadowRayInfo.desc.Direction = L;
    shadowRayInfo.flags = 0;
    shadowRayInfo.instanceMask = INSTANCE_OPAQUE | INSTANCE_TRANSLUCENT;
   
    radiance.rgb += currentMat.emissive;
    
    float shadowFactor = TraceVisibility(shadowRayInfo, g_Scene);
    radiance.rgb += EvaluateBRDF(currentMat.normal, L, V, currentMat) * g_DirectionalLight.color.w * g_DirectionalLight.color.rgb * shadowFactor;
    
    return true;
}

void IndirectLight(
    in float2 texCoords,
    RngStateType rngState, 
    inout float3 troughput, 
    inout float4 indirectDiffuse, 
    inout float4 indirectSpecular)
{
    GFragment fi = UnpackGBuffer(g_GBufferHeap, g_NearestRepeatSampler, texCoords);
    fi.albedo = pow(fi.albedo, 2.2f);
      
    SurfaceMaterial currentMat;
    currentMat.albedo = fi.albedo;
    currentMat.ao = fi.ao;
    currentMat.emissive = fi.emissive;
    currentMat.height = fi.height;
    currentMat.metallic = fi.metallic;
    currentMat.roughness = fi.roughness;
    currentMat.normal = fi.normal;
    
    float diffuseWeight = 0.f;
    float specWeight = 0.f;
    
    float3 geometryNormal = fi.geometryNormal;    
    float3 X = GetWorldPosFromDepth(texCoords, fi.depth, g_Camera.invViewProj);
    float3 L = -g_DirectionalLight.direction.rgb;
    float3 V = normalize(g_Camera.pos.rgb - X.rgb);
    float viewZ = g_GBufferHeap[GBUFFER_LINEAR_DEPTH].Sample(g_NearestRepeatSampler, texCoords).r;
  
    RayDesc ray;
    ray.TMin = 0.f;
    ray.TMax = 1e10f;
    ray.Origin = OffsetRay(X, fi.geometryNormal) + fi.geometryNormal * 0.05f;
    ray.Direction = -V;

    BRDFData gBufferBRDF = PrepareBRDFData(fi.normal, -g_DirectionalLight.direction.rgb, V, currentMat);
    
    float3 indirectRadiance = float3(0.f, 0.f, 0.f);
    
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
        opaqueRay.instanceMask = INSTANCE_OPAQUE | INSTANCE_TRANSLUCENT;
           
        HitAttributes hit;
        if (!CastRay(opaqueRay, g_Scene, hit))
        {
            indirectRadiance += troughput * SampleSky(ray.Direction, g_Cubemap, g_LinearRepeatSampler).rgb;
            indirectDiffuse.rgb += indirectRadiance * diffuseWeight;
            indirectSpecular.rgb += indirectRadiance * specWeight;
            
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
            indirectDiffuse.a = REBLUR_FrontEnd_GetNormHitDist(hit.minT, viewZ, g_RaytracingData.hitParams, fi.roughness) * diffuseWeight;
            indirectSpecular.a = REBLUR_FrontEnd_GetNormHitDist(hit.minT, viewZ, g_RaytracingData.hitParams, fi.roughness) * specWeight;
        }

        RayInfo shadowRayInfo;
        shadowRayInfo.desc.TMin = 0.00f;
        shadowRayInfo.desc.TMax = 10000.f;
        shadowRayInfo.desc.Origin = rayHitOffset;
        shadowRayInfo.desc.Direction = L;
        shadowRayInfo.flags = 0;
        shadowRayInfo.instanceMask = INSTANCE_OPAQUE | INSTANCE_TRANSLUCENT;

        indirectRadiance += currentMat.emissive * troughput;
        
        float shadowFactor = TraceVisibility(shadowRayInfo, g_Scene);
        indirectRadiance += troughput * EvaluateBRDF(hitSurfaceMaterial.normal, L, V, hitSurfaceMaterial) * g_DirectionalLight.color.w * g_DirectionalLight.color.rgb * shadowFactor;

        indirectDiffuse.rgb += indirectRadiance * diffuseWeight;
        indirectSpecular.rgb += indirectRadiance * specWeight;
        
        currentMat = hitSurfaceMaterial;
        ray.Origin = rayHitOffset;
    }
    
    float3 fenv = ApproxSpecularIntegralGGX(gBufferBRDF.specularF0, gBufferBRDF.alpha, gBufferBRDF.NdotV);
    float3 diffDemod = (1.f - fenv) * gBufferBRDF.diffuseReflectance;
    float3 specDemod = fenv;
    
    indirectDiffuse.rgb /= (diffDemod * 0.99f + 0.01f);
    indirectSpecular.rgb /= (specDemod * 0.99f + 0.01f);
    
    indirectDiffuse = REBLUR_FrontEnd_PackRadianceAndNormHitDist(indirectDiffuse.rgb, indirectDiffuse.a);
    indirectSpecular = REBLUR_FrontEnd_PackRadianceAndNormHitDist(indirectSpecular.rgb, indirectSpecular.a);
    
}

PixelShaderOutput main(float2 TexCoord : TEXCOORD)
{
    PixelShaderOutput OUT;
        
    uint2 pixelCoords = g_Camera.resolution * TexCoord;

    RngStateType rngState = InitRNG(pixelCoords, g_Camera.resolution, g_RaytracingData.frameNumber);
        
    RayDesc cameraRay = GeneratePinholeCameraRay(TexCoord * 2.f - 1.f);
    
    float3 directRadiance = float3(0.f, 0.f, 0.f);  
    float3 troughput = float3(1.f, 1.f, 1.f);
    
    //TranslucentPass(tranclucentRay, rngState, translucentTroughput, translucentRadiance);
    
    OUT.DirectLight = float4(0.f, 0.f, 0.f, 0.f);
    OUT.IndirectDiffuse = float4(0.f, 0.f, 0.f, 0.f);
    OUT.IndirectSpecular = float4(0.f, 0.f, 0.f, 0.f); 
    OUT.TransparentColor = float4(0.f, 0.f, 0.f, 0.f);
    OUT.rtDebug = float4(0.f, 0.f, 0.f, 0.f);
    
    if (!DirectLightGBuffer(TexCoord, rngState, OUT.DirectLight))
    {
        return OUT;
    }   
    IndirectLight(TexCoord, rngState, troughput, OUT.IndirectDiffuse, OUT.IndirectSpecular);
    
    /*
    
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
    
*/
    return OUT;
};

void TestRT(RayDesc ray, inout float3 debugColor)
{
    RayInfo rayInfo;
    rayInfo.desc = ray;
    rayInfo.instanceMask = INSTANCE_OPAQUE | INSTANCE_TRANSLUCENT;
    rayInfo.flags = 0;
    
    HitAttributes hit;
    if (!CastRay(rayInfo, g_Scene, hit))
        return;
      
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
