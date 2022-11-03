struct Mat
{
    matrix model; // Updates pr. object
    matrix mvp; // Updates pr. object
    matrix invTransposeMvp; // Updates pr. object
    
    matrix view; // Updates pr. frame
    matrix proj; // Updates pr. frame
    
    matrix invView; // Updates pr. frame
    matrix invProj; // Updates pr. frame
};

ConstantBuffer<Mat> MatCB : register(b0);

struct VertexPositionNormalTexture
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 PositionWS : POSITION;
    float3 NormalWS   : NORMAL;
    float2 TexCoord   : TEXCOORD;
    float4 Position   : SV_Position;
};

VertexShaderOutput main(VertexPositionNormalTexture IN)
{
    VertexShaderOutput OUT;
    OUT.Position = mul(MatCB.mvp, float4(IN.Position, 1.0f));
    OUT.PositionWS = mul(MatCB.model, float4(IN.Position, 1.f));
    OUT.NormalWS = mul((float3x3)MatCB.invTransposeMvp, IN.Normal);
    OUT.TexCoord = IN.TexCoord;
    
    return OUT;
}
