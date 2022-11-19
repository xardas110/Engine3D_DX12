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
#define COMPAT_VEC4F(v0, v1, v2, v3) = XMFLOAT4(v0, v1, v2, v3)
#define COMPAT_FLOAT(val) = val
#define COMPAT_INT(val) = val
#else 
#define COMPAT_ONE
#define COMPAT_VEC3F_ONE
#define COMPAT_VEC3F_NULL
#define COMPAT_FLOAT(val)
#define COMPAT_VEC4F(v0, v1, v2, v3)
#define COMPAT_INT(val)
#endif // !HLSL

#define TEXTURE_NULL UINT_MAX_NULL
#define MATERIAL_ID_NULL UINT_MAX_NULL

#define DEBUG_RAYTRACING_FINALCOLOR 0
#define DEBUG_RAYTRACING_ALBEDO 1
#define DEBUG_RAYTRACING_NORMAL 2
#define DEBUG_RAYTRACING_PBR    3
#define DEBUG_RAYTRACING_EMISSIVE 4

struct Material
{
    //Color with opacity
    XMFLOAT4 color COMPAT_VEC4F(1.f, 1.f, 1.f, 1.f);

    XMFLOAT3 emissive COMPAT_VEC3F_NULL;
    float shininess COMPAT_FLOAT(64.f);

    //Color for transperent objects
    XMFLOAT3 transparent COMPAT_VEC3F_ONE;
    float pad;
};

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
    UINT pad2 UINT_MAX_NULL;

    XMMATRIX transposeInverseModel;
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
    XMMATRIX invView;
    XMMATRIX invProj;
    XMMATRIX viewProj;
    XMMATRIX invViewProj;
    XMFLOAT3 pos;
    float pad;
    XMFLOAT2 resolution; //native res
    XMINT2 pad1;
};

struct RaytracingDataCB
{
    int debugSettings COMPAT_INT(-1); //-1 means nodebugging
    int frameNumber COMPAT_INT(0);
    int pad1; 
    int pad2;
};

#endif