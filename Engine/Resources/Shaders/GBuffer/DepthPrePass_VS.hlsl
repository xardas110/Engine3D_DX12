#define HLSL
#include "../Common/Types.h"

ConstantBuffer<ObjectCB> g_ObjectCB : register(b0);

struct VertexPositon
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 Position : SV_Position;
};

VertexShaderOutput main(VertexPositon IN)
{
    
    VertexShaderOutput OUT;
    OUT.Position = mul(g_ObjectCB.mvp, float4(IN.Position, 1.0f));
    return OUT;
}