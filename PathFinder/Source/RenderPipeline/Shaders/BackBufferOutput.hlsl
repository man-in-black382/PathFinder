struct PassData
{
    uint SourceTextureIndex;
};

#define PassDataType PassData

#include "BaseRootSignature.hlsl"

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

//void main() {
//    gl_Position = vec4(kVertices[gl_VertexID], -0.99, 1.0);
//    vTexCoords = gl_Position.xy / 2.0 + 0.5;
//}
//

VertexOut VSMain(uint vertexId : SV_VertexID)
{
    VertexOut output;

    output.Position = float4(Vertices[vertexId], 0, 1);
    output.UV = output.Position.xy / 2.0 + 0.5;

    return output;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    int3 coords = int3(pin.UV * float2(1280.0, 720.0), 0);
    Texture2D source = Textures2D[PassDataCB.SourceTextureIndex];
    float3 color = source.Load(coords);
    return float4(color, 1.0);
}