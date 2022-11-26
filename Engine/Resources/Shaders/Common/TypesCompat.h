#ifndef TYPES_H
#define TYPES_H

#ifdef HLSL
#include "HlslCompat.h"
#endif

#ifndef HLSL
#define UINT_MAX_NULL = 0xffffffff;
using namespace DirectX;
#else 
#define UINT_MAX_NULL
#endif // !HLSL

#ifndef HLSL
#define DEFAULT_NULL = 0;
#else 
#define DEFAULT_NULL
#endif // !HLSL

#ifndef HLSL
#define COMPAT_ONE = 1;
#define COMPAT_VEC3F_ONE = XMFLOAT3(1.f, 1.f, 1.f)
#define COMPAT_VEC3F_NULL = XMFLOAT3(0.f, 0.f, 0.f)
#define COMPAT_VEC3F(v0, v1, v2) = XMFLOAT3(v0, v1, v2)
#define COMPAT_VEC4F(v0, v1, v2, v3) = XMFLOAT4(v0, v1, v2, v3)
#define COMPAT_FLOAT(val) = val
#define COMPAT_INT(val) = val
#else 
#define COMPAT_ONE
#define COMPAT_VEC3F_ONE
#define COMPAT_VEC3F_NULL
#define COMPAT_FLOAT(val)
#define COMPAT_VEC3F(v0, v1, v2)
#define COMPAT_VEC4F(v0, v1, v2, v3)
#define COMPAT_INT(val)
#endif // !HLSL

#define TEXTURE_NULL UINT_MAX_NULL
#define MATERIAL_ID_NULL UINT_MAX_NULL

//0-7 color targets and 8 is depth
#define GBUFFER_ALBEDO 0
#define GBUFFER_NORMAL_ROUGHNESS 1
#define GBUFFER_MOTION_VECTOR 2
#define GBUFFER_EMISSIVE_SHADER_MODEL 3
#define GBUFFER_AO_METALLIC_HEIGHT 4
#define GBUFFER_LINEAR_DEPTH 5
#define GBUFFER_STANDARD_DEPTH 8

//RenderTargets 0-7 color
#define LIGHTBUFFER_DIRECT_LIGHT 0
#define LIGHTBUFFER_INDIRECT_DIFFUSE 1
#define LIGHTBUFFER_INDIRECT_SPECULAR 2
#define LIGHTBUFFER_RT_DEBUG 3
//UAV buffers
#define LIGHTBUFFER_ACCUM_BUFFER 4
#define LIGHTBUFFER_DENOISED_INDIRECT_DIFFUSE 5
#define LIGHTBUFFER_DENOISED_INDIRECT_SPECULAR 6

#define DEBUG_FINAL_COLOR 0
#define DEBUG_GBUFFER_ALBEDO 1
#define DEBUG_GBUFFER_NORMAL_ROUGHNESS 2
#define DEBUG_GBUFFER_MOTION_VECTOR 3
#define DEBUG_GBUFFER_EMISSIVE_SHADER_MODEL 4
#define DEBUG_GBUFFER_AO_METALLIC_HEIGHT 5
#define DEBUG_LINEAR_DEPTH 6

#define DEBUG_LIGHTBUFFER_DIRECT_LIGHT 7
#define DEBUG_LIGHTBUFFER_INDIRECT_DIFFUSE 8
#define DEBUG_LIGHTBUFFER_INDIRECT_SPECULAR 9
#define DEBUG_LIGHTBUFFER_DENOISED_INDIRECT_DIFFUSE 10
#define DEBUG_LIGHTBUFFER_DENOISED_INDIRECT_SPECULAR 11

#define DEBUG_RAYTRACED_ALBEDO 12
#define DEBUG_RAYTRACED_NORMAL 13
#define DEBUG_RAYTRACED_AO 14
#define DEBUG_RAYTRACED_ROUGHNESS 15
#define DEBUG_RAYTRACED_METALLIC 16
#define DEBUG_RAYTRACED_HEIGHT 17
#define DEBUG_RAYTRACED_EMISSIVE 18
//ShaderModels max 255 shader models
#define SM_SKY 0
#define SM_BRDF 1
#define SM_BSDF 2

struct Material
{
    //Color with opacity
    XMFLOAT4 color COMPAT_VEC4F(1.f, 1.f, 1.f, 1.f);

    XMFLOAT3 emissive COMPAT_VEC3F(1.f, 1.f, 1.f);
    float roughness COMPAT_FLOAT(1.f);
    
    //Color for transperent objects
    XMFLOAT3 transparent COMPAT_VEC3F(1.f, 1.f, 1.f);
    float metallic COMPAT_FLOAT(1.f);

};

#ifndef hlsl
    namespace MaterialType
    {
        enum Type
        {
            ao, albedo, normal, roughness, metallic, opacity, emissive, lightmap, height, NumMaterialTypes
        };
    }
#endif // !hlsl

struct MaterialInfo
{
    UINT ao TEXTURE_NULL;
    UINT albedo TEXTURE_NULL;
    UINT normal TEXTURE_NULL;
    UINT roughness TEXTURE_NULL;

    UINT metallic TEXTURE_NULL;
    UINT opacity TEXTURE_NULL;
    UINT emissive TEXTURE_NULL;
    UINT lightmap TEXTURE_NULL;

    UINT height TEXTURE_NULL;
    UINT pad2 TEXTURE_NULL;
    UINT materialID MATERIAL_ID_NULL; //User defined material
    UINT flags DEFAULT_NULL;
};

struct MeshInfo
{
    UINT vertexOffset UINT_MAX_NULL;
    UINT indexOffset UINT_MAX_NULL;
    UINT materialInstanceID UINT_MAX_NULL; //Cpu material index == Gpu material index
    UINT flags DEFAULT_NULL;
    XMVECTOR objRot; //quat to rotate normal
};

struct ObjectCB
{
    XMMATRIX model; // Updates pr. object
    XMMATRIX mvp; // Updates pr. object
    XMMATRIX invTransposeMvp; // Updates pr. object

    XMMATRIX view; // Updates pr. frame
    XMMATRIX proj; // Updates pr. frame

    XMMATRIX invView; // Updates pr. frame
    XMMATRIX invProj; // Updates pr. frame

    UINT entId UINT_MAX_NULL;
    UINT meshId UINT_MAX_NULL;
    UINT materialGPUID UINT_MAX_NULL; //for raster
    UINT shaderModel COMPAT_FLOAT(1.f);

    XMMATRIX transposeInverseModel;
    XMMATRIX invWorldToPrevWorld;
};

struct MeshVertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT2 textureCoordinate;
    XMFLOAT3 tangent;
    XMFLOAT3 bitangent;
};

struct DirectionalLightCB
{
    //Dir and ?
    XMVECTOR direction;
    //Color and itensity
    XMVECTOR color;
};

struct CameraCB
{
    XMMATRIX view;
    XMMATRIX proj;
    XMMATRIX prevView;
    XMMATRIX prevProj;
    XMMATRIX invView;
    XMMATRIX invProj;
    XMMATRIX viewProj;
    XMMATRIX invViewProj;
    XMFLOAT3 pos;
    float pad;
    XMFLOAT2 resolution; //native res
    float zNear;
    float zFar;
};

struct RaytracingDataCB
{
    int debugSettings COMPAT_INT(-1); //-1 means no debugging
    int frameNumber COMPAT_INT(0);
    int accumulatedFrameNumber; 
    int bResetDuty;
    int numBounces COMPAT_INT(2);
    int pad1;
    int pad2;
    int pad3;

};

#endif