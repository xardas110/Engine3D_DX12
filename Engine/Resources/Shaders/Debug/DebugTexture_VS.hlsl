struct VertexShaderOutput
{
    float2 TexCoord : TEXCOORD;
    float4 Position : SV_Position;
};

VertexShaderOutput main(uint VertexID : SV_VertexID)
{
    VertexShaderOutput OUT;
    
    // Calculate texture coordinates based on VertexID.
    // This approach creates full-screen quad coordinates from the SV_VertexID.
    // This is a common technique for full-screen quad rendering without using a vertex buffer.
    OUT.TexCoord = float2(uint2(VertexID, VertexID << 1) & 2);
    
    // Convert the texcoords into clip-space positions for a full-screen quad.
    // (-1,-1) is the bottom-left and (1,1) is the top-right in clip-space.
    OUT.Position = float4(lerp(float2(-1, 1), float2(1, -1), OUT.TexCoord), 0, 1);

    return OUT;
}
