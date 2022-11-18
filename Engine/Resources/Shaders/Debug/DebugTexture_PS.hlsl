Texture2D g_Texture : register(t0);
SamplerState g_NearestRepeatSampler : register(s0);

struct PixelShaderOutput
{
    float4 ColorTexture : SV_TARGET0;
};

PixelShaderOutput main(float2 TexCoord : TEXCOORD)
{
    PixelShaderOutput OUT;
    OUT.ColorTexture = g_Texture.Sample(g_NearestRepeatSampler, TexCoord);
    return OUT;
}