// Vertex Shader Output Structure
struct VertexShaderOutput
{
    float2 TexCoord : TEXCOORD; // Texture Coordinate
    float4 Position : SV_Position; // Screen Position
};

// Main Vertex Shader Function
VertexShaderOutput main(uint VertexID : SV_VertexID)
{
    VertexShaderOutput OUT;

    // Calculate texture coordinate based on VertexID. This method is common for full-screen quad shaders.
    OUT.TexCoord = float2(uint2(VertexID, VertexID << 1) & 2);

    // Interpolate between screen-space coordinates (-1,1) to (1,-1) using the TexCoord.
    // This arranges the vertices to cover the whole screen.
    OUT.Position = float4(lerp(float2(-1, 1), float2(1, -1), OUT.TexCoord), 0, 1);

    return OUT;
}