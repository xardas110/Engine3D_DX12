// Texture resource
Texture2D       g_Texture               : register(t0);

// Sampler state for nearest-neighbor sampling
SamplerState    g_NearestRepeatSampler : register(s0);

struct PixelShaderOutput
{
    float4 ColorTexture : SV_TARGET0;
};

// Main Pixel Shader function
PixelShaderOutput main(float2 TexCoord : TEXCOORD)
{
    PixelShaderOutput OUT;
    
    // Sample the texture using the provided texture coordinates
    OUT.ColorTexture = g_Texture.Sample(g_NearestRepeatSampler, TexCoord);
    
    return OUT;
}