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

ConstantBuffer<Material> MaterialCB : register(b0, space1);

struct PixelShaderInput
{
    float4 PositionWS : POSITION;
    float3 NormalWS : NORMAL;
    float2 TexCoord : TEXCOORD;
};

float4 main(PixelShaderInput IN) : SV_Target
{
    float4 out_col = float4(1.f, 0.f, 0.f, 1.f);
    return out_col;
}
