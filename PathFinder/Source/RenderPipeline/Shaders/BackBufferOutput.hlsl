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

VertexOut VSMain(uint vertexId : SV_VertexID)
{
    VertexOut output;

    output.UV = float2((vertexId << 1) & 2, vertexId & 2);
    output.Position = float4(output.UV  * float2(2, -2) + float2(-1, 1), 0, 1);

    return output;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    Texture2D source = Textures2D[PassDataCB.SourceTextureIndex];
    return float4(0.5, 0.5, 0.0, 1.0);
}