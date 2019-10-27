struct VertexOut
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

static const float2 Vertices[4] =
{
    float2(-1.0, 1.0),
    float2(-1.0, -1.0),
    float2(1.0, 1.0),
    float2(1.0, -1.0)
};

VertexOut VSMain(uint vertexId : SV_VertexID)
{
    VertexOut output;

    output.Position = float4(Vertices[vertexId], 0, 1);
    output.UV = output.Position.xy / 2.0 + 0.5;
    output.UV.y = 1.0 - output.UV.y; // Inverting Y because DirectX uses top left corner for {0,0} uv

    return output;
}