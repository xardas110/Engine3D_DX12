struct Material
{
    float4 Emissive;
    //----------------------------------- (16 byte boundary)
    float4 Ambient;
    //----------------------------------- (16 byte boundary)
    float4 Diffuse;
    //----------------------------------- (16 byte boundary)
    float4 Specular;
    //----------------------------------- (16 byte boundary)
    float SpecularPower;
    float3 Padding;
    //----------------------------------- (16 byte boundary)
    // Total:                              16 * 5 = 80 bytes
};

Texture2D DiffuseTexture : register(t0);
RaytracingAccelerationStructure Scene : register(t1, space0);
SamplerState LinearRepeatSampler : register(s0);
ConstantBuffer<Material> MaterialCB : register(b0, space1);

struct PixelShaderInput
{
    float4 PositionWS : POSITION;
    float3 NormalWS : NORMAL;
    float2 TexCoord : TEXCOORD;
};

float4 main(PixelShaderInput IN) : SV_Target
{
    float4 texColor = DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);
    return texColor;
}
