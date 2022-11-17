//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#ifndef RAYTRACINGHLSLCOMPAT_H
#define RAYTRACINGHLSLCOMPAT_H

#ifdef HLSL
#include "HlslCompat.h"
#else
using namespace DirectX;

#ifndef HLSL
#define UINT_MAX_NULL = 0xffffffff;
#else 
#define UINT_MAX_NULL
#endif // !HLSL

#ifndef HLSL
#define DEFAULT_NULL = 0;
#else 
#define DEFAULT_NULL
#endif // !HLSL

#define TEXTURE_NULL UINT_MAX_NULL
#define MATERIAL_ID_NULL UINT_MAX_NULL

// Shader will use byte encoding to access indices.
typedef UINT16 Index;
#endif

struct SceneConstantBuffer
{
    XMMATRIX projectionToWorld;
    XMVECTOR cameraPosition;
    XMVECTOR lightPosition;
    XMVECTOR lightAmbientColor;
    XMVECTOR lightDiffuseColor;
};

struct CubeConstantBuffer
{
    XMFLOAT4 albedo;
};

struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
};

struct MaterialInfo
{
    //Don't have space for flags here
    //Materialflags will be shared with meshflags
    //See meshinfo for flags
    UINT ao TEXTURE_NULL;
    UINT albedo TEXTURE_NULL;
    UINT normal TEXTURE_NULL;
    UINT roughness TEXTURE_NULL;

    UINT metallic TEXTURE_NULL;
    UINT opacity TEXTURE_NULL;
    UINT emissive TEXTURE_NULL;
    UINT lightmap TEXTURE_NULL;

    UINT materialID MATERIAL_ID_NULL; //User defined material
    UINT pad1 TEXTURE_NULL;
    UINT pad2 TEXTURE_NULL;
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

    UINT entId;
    UINT meshId;
    int pad1;
    int pad2;

    XMMATRIX transposeInverseModel;
};

struct MeshVertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT2 textureCoordinate;
};

#endif // RAYTRACINGHLSLCOMPAT_H