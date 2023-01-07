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

#define INSTANCE_OPAQUE (1<<0)
#define INSTANCE_TRANSLUCENT (1<<1)
#define INSTANCE_ALPHA_BLEND (1<<2)
#define INSTANCE_ALPHA_CUTOFF (1<<3)
#define INSTANCE_LIGHT (1<<7)
//Material flag has some shared data with instance flags
//TODO: FIX duplicates
#define MATERIAL_FLAG_BASECOLOR_ALPHA (1 << 4)
#define MATERIAL_FLAG_AO_ROUGH_METAL_AS_SPECULAR (1 << 5)
#define MATERIAL_FLAG_INVERT_NORMALS (1 << 6)

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

#define DENOISER_TEX_SCRAMBLING 0
#define DENOISER_TEX_SOBOL 1
#define DENOISER_TEX_NUM 2

#define LIGHT_INVALID 0
#define LIGHT_POINT 1
#define LIGHT_SPOT 2
#define LIGHT_DIRECTIONAL 3
#define MAX_LIGHTS 4

// Number of candidates used for resampling of analytical lights
#define RIS_CANDIDATES_LIGHTS MAX_LIGHTS

//0-7 color targets and 8 is depth
#define GBUFFER_ALBEDO 0
#define GBUFFER_NORMAL_ROUGHNESS 1
#define GBUFFER_MOTION_VECTOR 2
#define GBUFFER_EMISSIVE_SHADER_MODEL 3
#define GBUFFER_AO_METALLIC_HEIGHT 4
#define GBUFFER_LINEAR_DEPTH 5
#define GBUFFER_GEOMETRY_NORMAL 6
#define GBUFFER_GEOMETRY_MV2D 7
#define GBUFFER_STANDARD_DEPTH 8
#define GBUFFER_NUM 9

//RenderTargets 0-7 color
#define LIGHTBUFFER_DIRECT_LIGHT 0
#define LIGHTBUFFER_INDIRECT_DIFFUSE 1
#define LIGHTBUFFER_INDIRECT_SPECULAR 2
#define LIGHTBUFFER_RT_DEBUG 3
#define LIGHTBUFFER_SHADOW_DATA 4
#define LIGHTBUFFER_TRANSPARENT_COLOR 7
//UAV buffers
#define LIGHTBUFFER_DENOISED_INDIRECT_DIFFUSE 5
#define LIGHTBUFFER_DENOISED_INDIRECT_SPECULAR 6
#define LIGHTBUFFER_DENOISED_SHADOW 8
#define LIGHTBUFFER_NUM 9

#define DEBUG_FINAL_COLOR 0
#define DEBUG_GBUFFER_ALBEDO 1
#define DEBUG_GBUFFER_NORMAL_ROUGHNESS 2
#define DEBUG_GBUFFER_MOTION_VECTOR 3
#define DEBUG_GBUFFER_EMISSIVE_SHADER_MODEL 4
#define DEBUG_GBUFFER_AO_METALLIC_HEIGHT 5
#define DEBUG_LINEAR_DEPTH 6
#define DEBUG_GEOMETRY_NORMAL 7

#define DEBUG_LIGHTBUFFER_DIRECT_LIGHT 8
#define DEBUG_LIGHTBUFFER_INDIRECT_DIFFUSE 9
#define DEBUG_LIGHTBUFFER_INDIRECT_SPECULAR 10
#define DEBUG_LIGHTBUFFER_DENOISED_INDIRECT_DIFFUSE 11
#define DEBUG_LIGHTBUFFER_DENOISED_INDIRECT_SPECULAR 12

#define DEBUG_RAYTRACED_ALBEDO 13
#define DEBUG_RAYTRACED_NORMAL 14
#define DEBUG_RAYTRACED_AO 15
#define DEBUG_RAYTRACED_ROUGHNESS 16
#define DEBUG_RAYTRACED_METALLIC 17
#define DEBUG_RAYTRACED_HEIGHT 18
#define DEBUG_RAYTRACED_EMISSIVE 19
#define DEBUG_RAYTRACED_HIT_T 20

#define DEBUG_GBUFFER_MV2D 21
#define DEBUG_LIGHTBUFFER_TRANSLUCENT 22

//ShaderModels max 255 shader models
#define SM_SKY 0
#define SM_BRDF 1
#define SM_BSDF 2

// Tonemapping methods
#define TM_Linear     0
#define TM_Reinhard   1
#define TM_ReinhardSq 2
#define TM_ACESFilmic 3
#define TM_Uncharted 4

#define HISTOGRAM_BINS 256
#define FIXED_POINT_FRAC_BITS 6
#define FIXED_POINT_FRAC_MULTIPLIER (1 << FIXED_POINT_FRAC_BITS)

#define HISTROGRAM_GROUP_X 16
#define HISTROGRAM_GROUP_Y 16

// Converts Phong's exponent (shininess) to Beckmann roughness (alpha)
// Source: "Microfacet Models for Refraction through Rough Surfaces" by Walter et al.
inline float ShininessToBeckmannAlpha(float shininess)
{
    return sqrt(2.0f / (shininess + 2.0f));
}

// Converts Beckmann roughness (alpha) to Oren-Nayar roughness (sigma)
// Source: "Moving Frostbite to Physically Based Rendering" by Lagarde & de Rousiers
inline float BeckmannAlphaToOrenNayarRoughness(float alpha)
{
    return 0.7071067f * atan(alpha);
}

struct TonemapCB
{
    // The method to use to perform tonemapping.
    UINT TonemapMethod COMPAT_INT(TM_ReinhardSq);
    // Exposure should be expressed as a relative expsure value (-2, -1, 0, +1, +2 )
    float Exposure COMPAT_FLOAT(2.f);

    // The maximum luminance to use for linear tonemapping.
    float MaxLuminance COMPAT_FLOAT(1.f);

    // Reinhard constant. Generlly this is 1.0.
    float K COMPAT_FLOAT(1.f);

    // ACES Filmic parameters
    // See: https://www.slideshare.net/ozlael/hable-john-uncharted2-hdr-lighting/142
    float A COMPAT_FLOAT(0.22f); // Shoulder strength
    float B COMPAT_FLOAT(0.3f); // Linear strength
    float C COMPAT_FLOAT(0.1f); // Linear angle
    float D COMPAT_FLOAT(0.2f); // Toe strength
    float E COMPAT_FLOAT(0.01f);; // Toe Numerator
    float F COMPAT_FLOAT(0.3f);; // Toe denominator
    // Note E/F = Toe angle.
    float LinearWhite COMPAT_FLOAT(11.2f);

    float Gamma COMPAT_FLOAT(2.2f);

    XMFLOAT2 viewOrigin;
    XMFLOAT2 viewSize;

    float logLuminanceScale;
    float logLuminanceBias;
    float histogramLowPercentile;
    float histogramHighPercentile;

    float eyeAdaptationSpeedUp;
    float eyeAdaptationSpeedDown;
    float minAdaptedLuminance;
    float maxAdaptedLuminance;

    float frameTime;
    float exposureScale;
    float whitePointInvSquared;
    UINT sourceSlice;

    float minLogLuminance COMPAT_FLOAT(-10.f); // TODO: figure out how to set these properly
    float maxLogLuminamce COMPAT_FLOAT(4.f);
    int bEyeAdaption COMPAT_INT(1);
    float pad2;
};

struct Light
{
    XMFLOAT3 pos;
    UINT type;
    XMFLOAT3 intensity;
    UINT pad;
};

struct LightDataCB
{
    UINT numLights;
    UINT p0;
    UINT p1;
    UINT p2;

    Light lights[MAX_LIGHTS];
};

struct Material
{
    //Color with opacity
    XMFLOAT4 diffuse COMPAT_VEC4F(1.f, 0.f, 0.f, 1.f);

    //specular shininess
    XMFLOAT4 specular COMPAT_VEC4F(1.f, 1.f, 1.f, 0.f);

    XMFLOAT3 emissive COMPAT_VEC3F(1.f, 1.f, 1.f);
    float roughness COMPAT_FLOAT(0.5f);
    
    //Color for transperent objects
    XMFLOAT3 transparent COMPAT_VEC3F(1.f, 1.f, 1.f);
    float metallic COMPAT_FLOAT(0.5f);
};

#ifndef HLSL
    namespace MaterialType
    {
        enum Type
        {
            ao, albedo, normal, roughness, metallic, opacity, emissive, lightmap, height, NumMaterialTypes
        };

        inline const char* typeNames[NumMaterialTypes] =
        {
            "ao", "albedo", "normal", "roughness", "metallic", "opacity", "emissive", "lightmap", "height"
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
    UINT specular TEXTURE_NULL;
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
    XMMATRIX prevModel;
    XMMATRIX prevMVP;
    XMVECTOR objRotQuat;
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

    float azimuth COMPAT_FLOAT(-147.f);
    float elevation COMPAT_FLOAT(45.f);
    float angularDiameter COMPAT_FLOAT(0.533f);
    float tanAngularRadius; //Tan( DegToRad( m_Settings.sunAngularDiameter * 0.5f ) ); in application
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
    float eyeToPixelConeSpreadAngle COMPAT_FLOAT(0.f); //Temp, might move 
    XMFLOAT2 resolution; //native res
    float zNear;
    float zFar;

    XMFLOAT2 jitter;
    float pad1;
    float pad2;
};

struct RaytracingDataCB
{
    int debugSettings COMPAT_INT(-1); //-1 means no debugging
    UINT frameNumber COMPAT_INT(0);
    int accumulatedFrameNumber; 
    int bResetDuty;
    int numBounces COMPAT_INT(2);
    int frameIndex;
    float pad;
    float oneBounceAmbientStrength;
    XMFLOAT4 hitParams;
};

#endif