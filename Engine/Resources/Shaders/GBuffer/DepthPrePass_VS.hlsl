#define HLSL
#include "../Common/TypesCompat.h"

ConstantBuffer<ObjectCB> g_ObjectCB : register(b0);

struct VertexPosition
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 Position : SV_Position;
};

VertexShaderOutput main(VertexPosition IN)
{
    VertexShaderOutput OUT;
    OUT.Position = mul(g_ObjectCB.mvp, float4(IN.Position, 1.0f));
    return OUT;
};