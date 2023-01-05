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

Texture2D<uint4>                g_BlueNoise[]               : register(t8, space9);
    
SamplerState                    g_NearestRepeatSampler      : register(s0);
SamplerState                    g_LinearRepeatSampler       : register(s1);
SamplerState                    g_LinearClampSampler        : register(s2);

ConstantBuffer<DirectionalLightCB> g_DirectionalLight       : register(b0);
ConstantBuffer<CameraCB>           g_Camera                 : register(b1);
ConstantBuffer<RaytracingDataCB>   g_RaytracingData         : register(b2);
ConstantBuffer<LightDataCB>        g_LightData              : register(b3);

RWTexture2D<float4>             g_GlobalUAV[]               : register(u0);
  
static float reflectiveAmbient = 0.005f;

#define DIRECT_LIGHT_FLAG_SKIP_SKY (1 << 0)
#define DIRECT_LIGHT_FLAG_SKIP_OPAQUE (1 << 1)
#define DIRECT_LIGHT_FLAG_SKIP_CUTOFF (1 << 2)
#define DIRECT_LIGHT_FLAG_SKIP_BLEND (1 << 3)
#define DIRECT_LIGHT_FLAG_SAVE_OPACITY (1 << 4)
#define DIRECT_LIGHT_FLAG_HARD_SHADOWS (1 << 5)
#define DIRECT_LIGHT_FLAG_SKIP_SHADOWS (1 << 6)

#define ANY_HIT_FLAGS_SKIP_BLEND (1 << 0)

#define FLT_MAX 3.402823466e+38F

struct PixelShaderOutput
{
    float4 DirectLight      : SV_TARGET0;
    float4 IndirectDiffuse  : SV_TARGET1;
    float4 IndirectSpecular : SV_TARGET2;
    float4 rtDebug          : SV_TARGET3;
    float4 TransparentColor : SV_TARGET4;
    float2 ShadowData       : SV_TARGET5;
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

struct VisibilityResult
{
    bool bHit;
    float hitT;
};

void TestRT(RayDesc ray, inout float3 debugColor);

//Raytracing gems II chapter 24
float2 MapRectToCircle(in float2 rect)
{      
    float radius = sqrt(rect.x);      
    float angle = rect.y * 2.0 * 3.14159265359;
        
    return float2(
        radius * cos(angle),
        radius * sin(angle)
    );
}

float3 SphericalDirectionalLightRayDirection(in float2 rect, in float3 direction, in float radius)
{
    float2 p = MapRectToCircle(rect) * radius;

    float3 tangent = normalize(cross(direction, float3(0.0, 1.0, 0.0)));
    
    float3 bitangent = normalize(cross(tangent, direction));
    
    return normalize(direction + p.x * tangent + p.y * bitangent);
}

//Raytracing gems II Chapter 14, 20, 22
// Returns intensity of given light at specified distance
float3 GetLightIntensityAtPoint(Light light, float distance)
{
    if (light.type == LIGHT_POINT)
    {
		// Cem Yuksel's improved attenuation avoiding singularity at distance=0
		// Source: http://www.cemyuksel.com/research/pointlightattenuation/
        const float radius = 0.5f; //< We hardcode radius at 0.5, but this should be a light parameter
        const float radiusSquared = radius * radius;
        const float distanceSquared = distance * distance;
        const float attenuation = 2.0f / (distanceSquared + radiusSquared + distance * sqrt(distanceSquared + radiusSquared));

        return light.intensity * attenuation;
    }
    else if (light.type == LIGHT_DIRECTIONAL)
    {
        return light.intensity;
    }
    else
    {
        return float3(1.0f, 0.0f, 1.0f);
    }
}

// Decodes light vector and distance from Light structure based on the light type
void GetLightData(Light light, float3 hitPosition, out float3 lightVector, out float lightDistance)
{
    if (light.type == LIGHT_POINT)
    {
        lightVector = light.pos - hitPosition;
        lightDistance = length(lightVector);
    }
    else if (light.type == LIGHT_DIRECTIONAL)
    {
        lightVector = light.pos; //< We use position field to store direction for directional light
        lightDistance = FLT_MAX;
    }
    else
    {
        lightDistance = FLT_MAX;
        lightVector = float3(0.0f, 1.0f, 0.0f);
    }
}

//Raytracing gems II Chapter 14, 20, 22
bool SampleLightUniform(inout RngStateType rngState, float3 hitPosition, float3 surfaceNormal, out Light light, out float lightSampleWeight)
{
    if (g_LightData.numLights == 0)
        return false;

    uint randomLightIndex = min(g_LightData.numLights - 1, uint(Rand(rngState) * g_LightData.numLights));
    light = g_LightData.lights[randomLightIndex];

	// PDF of uniform distribution is (1/light count). Reciprocal of that PDF (simply a light count) is a weight of this sample
    lightSampleWeight = float(g_LightData.numLights);

    return true;
}

//Raytracing gems II Chapter 14, 20, 22
bool SampleLightRIS(inout RngStateType rngState, float3 hitPosition, float3 surfaceNormal, out Light selectedSample, out float lightSampleWeight)
{	
    if (g_LightData.numLights == 0)
        return false;

    selectedSample = (Light) 0;
    float totalWeights = 0.0f;
    float samplePdfG = 0.0f;

    for (int i = 0; i < RIS_CANDIDATES_LIGHTS; i++)
    {
        float candidateWeight;
        Light candidate;
        if (SampleLightUniform(rngState, hitPosition, surfaceNormal, candidate, candidateWeight))
        {

            float3 lightVector;
            float lightDistance;
            GetLightData(candidate, hitPosition, lightVector, lightDistance);

			// Ignore backfacing light
            float3 L = normalize(lightVector);
            if (dot(surfaceNormal, L) < 0.00001f)
                continue;

            float candidatePdfG = Luminance(GetLightIntensityAtPoint(candidate, length(lightVector)));
            const float candidateRISWeight = candidatePdfG * candidateWeight;

            totalWeights += candidateRISWeight;
            if (Rand(rngState) < (candidateRISWeight / totalWeights))
            {
                selectedSample = candidate;
                samplePdfG = candidatePdfG;
            }
        }
    }

    if (totalWeights == 0.0f)
    {
        return false;
    }
    else
    {
        lightSampleWeight = (totalWeights / float(RIS_CANDIDATES_LIGHTS)) / samplePdfG;
        return true;
    }
}

// https://en.wikipedia.org/wiki/Ordered_dithering
uint Bayer4x4ui( uint2 samplePos, uint frameIndex)
{
    uint2 samplePosWrap = samplePos & 3;
    uint a = 2068378560 * ( 1 - ( samplePosWrap.x >> 1 ) ) + 1500172770 * ( samplePosWrap.x >> 1 );
    uint b = ( samplePosWrap.y + ( ( samplePosWrap.x & 1 ) << 2 ) ) << 2;

    uint sampleOffset = frameIndex;

    return ( ( a >> b ) + sampleOffset ) & 0xF;
}

//Raytracing gems II chapter 24
float AnimateBlueNoise(in float blueNoise, in int frameIndex)
{
    return frac(blueNoise + float(frameIndex % 32) * 0.61803399);
}

//https://github.com/NVIDIAGameWorks/RayTracingDenoiser
// + Raytracing gems II Chapter 24 for animated blue noise
float2 GetBlueNoise(uint2 pixelPos, uint seed, uint sampleIndex, uint sppVirtual = 1, uint spp = 1)
{
    // Final SPP - total samples per pixel ( there is a different "gIn_Scrambling_Ranking" texture! )
    // SPP - samples per pixel taken in a single frame ( must be POW of 2! )
    // Virtual SPP - "Final SPP / SPP" - samples per pixel distributed in time ( across frames )

    // Based on:
    //     https://eheitzresearch.wordpress.com/772-2/
    // Source code and textures can be found here:
    //     https://belcour.github.io/blog/research/publication/2019/06/17/sampling-bluenoise.html (but 2D only)

    // Sample index
    uint frameIndex = g_RaytracingData.frameNumber;
    uint virtualSampleIndex = (frameIndex + seed) & (sppVirtual - 1);
    sampleIndex &= spp - 1;
    sampleIndex += virtualSampleIndex * spp;

    // The algorithm
    uint3 A = g_BlueNoise[DENOISER_TEX_SCRAMBLING][pixelPos & 127].rgb;
    uint rankedSampleIndex = sampleIndex ^ A.z;
    uint4 B = g_BlueNoise[DENOISER_TEX_SOBOL][uint2(rankedSampleIndex & 255, 0)];
    float4 blue = (float4(B ^ A.xyxy) + 0.5) * (1.0 / 256.0);

    // Randomize in [ 0; 1 / 256 ] area to get rid of possible banding
    uint d = Bayer4x4ui(pixelPos, frameIndex);
    float2 dither = (float2(d & 3, d >> 2) + 0.5) * (1.0 / 4.0);
    blue += (dither.xyxy - 0.5) * (1.0 / 256.0);

    return saturate(blue.xy);
}

bool TestOpacity(in HitAttributes hit, inout float opacity, float cutOff = 0.5f, uint anyhitFlags = 0)
{
    MeshInfo meshInfo = g_GlobalMeshInfo[hit.instanceIndex];
    MaterialInfo materialInfo = g_GlobalMaterialInfo[meshInfo.materialInstanceID];
         
    MeshVertex hitSurface = GetHitSurface(hit, meshInfo, g_GlobalMeshVertexData, g_GlobalMeshIndexData);

    SurfaceMaterial hitSurfaceMaterial = GetSurfaceMaterial(materialInfo, hitSurface, hit.objToWorld, meshInfo.objRot, g_LinearRepeatSampler, g_GlobalTextureData, 0.f, SAMPLE_TYPE_LEVEL);
    ApplyMaterial(materialInfo, hitSurfaceMaterial, g_GlobalMaterials);

    opacity = hitSurfaceMaterial.opacity;
    
    if (hitSurfaceMaterial.opacity < cutOff)
        return true;
 
    if (ANY_HIT_FLAGS_SKIP_BLEND & anyhitFlags && materialInfo.flags & INSTANCE_ALPHA_BLEND)
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
        if (!TestOpacity(hit, opacity, 0.5f, ray.anyhitFlags))      
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

float TraceVisibility(RayInfo ray, RaytracingAccelerationStructure scene, out VisibilityResult result)
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
    }
    
    result.bHit = shadowQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
    
    if (!result.bHit)
        return visibility;
     
    result.hitT = shadowQuery.CommittedRayT();
    
    return 0.f;
}

//Returns true on refraction
bool TraceDirectLight(RayInfo ray, RngStateType rng, float ambientFactor, uint flags, inout float3 troughput, inout float4 radiance, inout TraceResult traceResult)
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
        
    if (flags & DIRECT_LIGHT_FLAG_SKIP_CUTOFF && materialInfo.flags & INSTANCE_ALPHA_CUTOFF)
        return false;
    
    if (flags & DIRECT_LIGHT_FLAG_SKIP_BLEND && materialInfo.flags & INSTANCE_ALPHA_BLEND)
        return false;
       
    //float2 aUVs[3];
    //float3 aPos[3];   
    MeshVertex hitSurface = GetHitSurface(hit, meshInfo, g_GlobalMeshVertexData, g_GlobalMeshIndexData); // aUVs, aPos);
    
    float3 rayHit = mul(hit.objToWorld, float4(hitSurface.position, 1.f));
    float3 geometryNormal = RotatePoint(meshInfo.objRot, hitSurface.normal);
    
    //For ray cones
    //float4 texLodGrad = RayConeGradientMIP(ray.desc.Direction, geometryNormal, hit.minT, g_Camera.eyeToPixelConeSpreadAngle, aUVs, aPos, (float3x3) hit.objToWorld);
    
    SurfaceMaterial hitSurfaceMaterial = GetSurfaceMaterial(materialInfo, hitSurface, hit.objToWorld, meshInfo.objRot, g_LinearRepeatSampler, g_GlobalTextureData, 0.f, SAMPLE_TYPE_MIP);
    
    ApplyMaterial(materialInfo, hitSurfaceMaterial, g_GlobalMaterials);

    if (flags & DIRECT_LIGHT_FLAG_SAVE_OPACITY)
        radiance.a = hitSurfaceMaterial.opacity;
        
    float3 V = normalize(-ray.desc.Direction);                            
    float3 L = -g_DirectionalLight.direction.rgb;
    float3 X = rayHit;
     
    float3 rayHitOffset = OffsetRay(X, geometryNormal);

    traceResult.mat = hitSurfaceMaterial;
    traceResult.hit = hit;
    traceResult.geoNormal = geometryNormal;
    traceResult.vert = hitSurface;
    traceResult.hitPos = rayHitOffset;
                               
    Light selectedLight;
    float lightWeight;
    if (SampleLightRIS(rng, X, geometryNormal, selectedLight, lightWeight))
    {
        float3 lightVector;
        float lightDistance;
        GetLightData(selectedLight, X, lightVector, lightDistance);
        float3 L = normalize(lightVector);
    
        RayInfo shadowRayInfo;
        shadowRayInfo.desc.TMin = 0.0f;
        shadowRayInfo.desc.TMax = lightDistance;
        shadowRayInfo.desc.Origin = X;
        shadowRayInfo.desc.Direction = L;
        shadowRayInfo.flags = 0;
        shadowRayInfo.anyhitFlags = 0;
        shadowRayInfo.instanceMask = INSTANCE_OPAQUE | INSTANCE_TRANSLUCENT;
   
        VisibilityResult visibilityResult;
   
        float visibility = TraceVisibility(shadowRayInfo, g_Scene, visibilityResult);
    
        radiance.rgb += EvaluateBRDF(traceResult.mat.normal, L, V, traceResult.mat) * (GetLightIntensityAtPoint(selectedLight, lightDistance) * lightWeight) * visibility;
    }
    
    return true;
}

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

bool TranslucentPass(in float2 texcoords, RngStateType rng, float3 troughput, inout float4 radiance)
{
    RayInfo ray;
    ray.desc = GeneratePinholeCameraRay(texcoords * 2.f - 1.f);
    ray.desc.TMin = 0.1f;
    ray.flags = 0;
    ray.anyhitFlags = 0;
    ray.instanceMask = INSTANCE_OPAQUE | INSTANCE_TRANSLUCENT; 
        
    TraceResult traceResult;
    bool bResult = TraceDirectLight(ray, rng, 0.001f, DIRECT_LIGHT_FLAG_SAVE_OPACITY | DIRECT_LIGHT_FLAG_SKIP_OPAQUE | DIRECT_LIGHT_FLAG_SKIP_CUTOFF | DIRECT_LIGHT_FLAG_SKIP_SKY, troughput, radiance, traceResult);
       
    if (!bResult)
        return bResult;
      
    if (traceResult.mat.roughness < 0.1f)
    {
        float brdfProbability = GetBRDFProbability(traceResult.mat, -ray.desc.Direction, traceResult.mat.normal);
        troughput /= float3(brdfProbability, brdfProbability, brdfProbability);
        
        float3 brdfWeight;
        float2 u = float2(Rand(rng), Rand(rng));
        if (EvaluateIndirectBRDF(u, traceResult.mat.normal, traceResult.geoNormal, -ray.desc.Direction, traceResult.mat, BRDF_TYPE_SPECULAR, ray.desc.Direction, brdfWeight, rng))
        {           
            ray.desc.Origin = traceResult.hitPos;  
            troughput *= brdfWeight;
            TraceDirectLight(ray, rng, 0.004f, DIRECT_LIGHT_FLAG_HARD_SHADOWS, troughput, radiance, traceResult);
        }
    }
 
    return bResult;
}

float3x3 GetBasis(float3 N)
{
    float sz = sign(N.z);
    float a = 1.0 / (sz + N.z);
    float ya = N.y * a;
    float b = N.x * ya;
    float c = N.x * sz;

    float3 T = float3(c * N.x * a - 1.0, sz * b, c);
    float3 B = float3(b, N.y * ya - sz, N.y);

            // Note: due to the quaternion formulation, the generated frame is rotated by 180 degrees,
            // s.t. if N = (0, 0, 1), then T = (-1, 0, 0) and B = (0, -1, 0).
    return float3x3(T, B, N);
}

bool DirectLightGBuffer(in float2 texCoords, RngStateType rng, float viewZ, inout float4 radiance, inout float2 shadowData)
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

    if (fi.shaderModel == SM_SKY)
    {
        radiance.rgb = SampleSky(-V, g_Cubemap, g_LinearClampSampler);
        return false;
    }
     
    Light selectedLight;
    float lightWeight;    
    if (SampleLightRIS(rng, X, fi.geometryNormal, selectedLight, lightWeight))
    {      
        float3 lightVector;
        float lightDistance;
        GetLightData(selectedLight, X, lightVector, lightDistance);
        float3 L = normalize(lightVector);
    
        radiance.a = lightDistance;
        
        RayInfo shadowRayInfo;
        shadowRayInfo.desc.TMin = 0.0f;
        shadowRayInfo.desc.TMax = lightDistance;
        shadowRayInfo.desc.Origin = X;
        shadowRayInfo.desc.Direction = L;
        shadowRayInfo.flags = 0;
        shadowRayInfo.anyhitFlags = 0;
        shadowRayInfo.instanceMask = INSTANCE_OPAQUE | INSTANCE_TRANSLUCENT;
   
        uint2 pixelPos = texCoords * g_Camera.resolution;
        float2 rnd = GetBlueNoise(pixelPos, 0, 0);

        //shadowRayInfo.desc.Direction = SphericalDirectionalLightRayDirection(rnd, L, g_DirectionalLight.tanAngularRadius);
    
        VisibilityResult visibilityResult;
   
        float visibility = TraceVisibility(shadowRayInfo, g_Scene, visibilityResult);
    
        /*
        if (visibilityResult.bHit)
            shadowData = SIGMA_FrontEnd_PackShadow(viewZ, visibilityResult.hitT, 0.f);
        else
            shadowData = SIGMA_FrontEnd_PackShadow(viewZ, lightDistance, 0.f);
        */
    
        //radiance.rgb += fi.emissive; Has to be added at composition stage since shadow is seperated for denoising
        radiance.rgb += EvaluateBRDF(currentMat.normal, L, V, currentMat) * (GetLightIntensityAtPoint(selectedLight, lightDistance) * lightWeight) * visibility;
    }
    
    return true;
}

void IndirectLight(
    in float2 texCoords,
    RngStateType rngState,
    float viewZ,
    inout float4 indirectRadiance,
    inout float3 troughput, 
    inout float4 indirectDiffuse, 
    inout float4 indirectSpecular)
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
    
    float diffuseWeight = 0.f;
    float specWeight = 0.f;
       
    float3 X = GetWorldPosFromDepth(texCoords, fi.depth, g_Camera.invViewProj);
    float3 L = -g_DirectionalLight.direction.rgb;
    float3 V = normalize(g_Camera.pos.rgb - X.rgb);    
    float3 geometryNormal = fi.geometryNormal;
    
    RayDesc ray;
    ray.TMin = 0.0f;
    ray.TMax = 1e10f;
    ray.Origin = OffsetRay(X, fi.geometryNormal) + fi.geometryNormal * 0.05f;
    ray.Direction = -V;
        
    BRDFData gBufferBRDF = PrepareBRDFData(fi.normal, -g_DirectionalLight.direction.rgb, V, currentMat);
    
    for (int i = 0; i < g_RaytracingData.numBounces; i++)
    {       
        if (dot(geometryNormal, V) < 0.0f)
            geometryNormal = -geometryNormal;
        if (dot(geometryNormal, currentMat.normal) < 0.0f)
            currentMat.normal = -currentMat.normal;
        
        int brdfType;
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
              
        float3 brdfWeight;
        float2 u = float2(Rand(rngState), Rand(rngState));
        if (!EvaluateIndirectBRDF(u, currentMat.normal, geometryNormal, V, currentMat, brdfType, ray.Direction, brdfWeight, rngState))
        {
            break;
        }
           
        troughput *= brdfWeight;
        V = -ray.Direction;
               
        RayInfo indirectRay;
        indirectRay.desc = ray;
        indirectRay.flags = 0;
        indirectRay.anyhitFlags = ANY_HIT_FLAGS_SKIP_BLEND;
        indirectRay.instanceMask = INSTANCE_OPAQUE | INSTANCE_TRANSLUCENT;
           
        TraceResult traceResult;
        if (!TraceDirectLight(indirectRay, rngState, g_RaytracingData.numBounces > 1? 0.f : 0.001f, 0, troughput, indirectRadiance, traceResult))
        {
            indirectDiffuse.rgb += indirectRadiance.rgb * diffuseWeight;
            indirectSpecular.rgb += indirectRadiance.rgb * specWeight;      
            
            if (i == 0)
            {
                indirectDiffuse.a = REBLUR_FrontEnd_GetNormHitDist(50000.f, viewZ, g_RaytracingData.hitParams, fi.roughness) * diffuseWeight;
                indirectSpecular.a = REBLUR_FrontEnd_GetNormHitDist(50000.f, viewZ, g_RaytracingData.hitParams, fi.roughness) * specWeight;
            }
            
            break;
        }
                     
        if (i == 0)
        {
            indirectDiffuse.a = REBLUR_FrontEnd_GetNormHitDist(traceResult.hit.minT, viewZ, g_RaytracingData.hitParams, fi.roughness) * diffuseWeight;
            indirectSpecular.a = REBLUR_FrontEnd_GetNormHitDist(traceResult.hit.minT, viewZ, g_RaytracingData.hitParams, fi.roughness) * specWeight;
        }

        indirectDiffuse.rgb += indirectRadiance * diffuseWeight;
        indirectSpecular.rgb += indirectRadiance * specWeight;
        
        currentMat = traceResult.mat;
        ray.Origin = traceResult.hitPos;
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

    float viewZ = g_GBufferHeap[GBUFFER_LINEAR_DEPTH].Sample(g_NearestRepeatSampler, TexCoord).r;
    
    RngStateType rngState = InitRNG(pixelCoords, g_Camera.resolution, g_RaytracingData.frameNumber);
        
    float3 directRadiance = float3(0.f, 0.f, 0.f);  
    float3 troughput = float3(1.f, 1.f, 1.f);
    
    OUT.DirectLight = float4(0.f, 0.f, 0.f, 0.f);
    OUT.IndirectDiffuse = float4(0.f, 0.f, 0.f, 0.f);
    OUT.IndirectSpecular = float4(0.f, 0.f, 0.f, 0.f); 
    OUT.TransparentColor = float4(0.f, 0.f, 0.f, 0.f);
    OUT.rtDebug = float4(0.f, 0.f, 0.f, 0.f);

    TranslucentPass(TexCoord, rngState, float3(1.f, 1.f, 1.f), OUT.TransparentColor);
     
    if (!DirectLightGBuffer(TexCoord, rngState, viewZ, OUT.DirectLight, OUT.ShadowData))
    {
        return OUT;
    }   
    float4 indirectLight = 0.f;
    IndirectLight(TexCoord, rngState, viewZ, OUT.DirectLight, troughput, OUT.IndirectDiffuse, OUT.IndirectSpecular);
    
    if (DEBUG_FINAL_COLOR == g_RaytracingData.debugSettings)
    {
        return OUT;
    }
    RayDesc ray = GeneratePinholeCameraRay(TexCoord * 2.f - 1.f);
    ray.TMax = 10000.f;
    
    float3 rtDebugColor = float3(0.f, 0.f, 0.f);
    TestRT(ray, rtDebugColor);
    
    OUT.rtDebug = float4(rtDebugColor, 1.f);
    
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
