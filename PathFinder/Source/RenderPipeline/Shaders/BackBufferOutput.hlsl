struct PassData
{
    uint SourceTextureIndex;
};

#define PassDataType PassData

#include "BaseRootSignature.hlsl"
#include "FullScreenQuadVS.hlsl"

float4 PSMain(VertexOut pin) : SV_Target
{
    int2 coords = int2(pin.UV * float2(1280.0, 720.0));
    Texture2D source = Textures2D[PassDataCB.SourceTextureIndex];
    return float4(source[coords].rgb, 1.0);
}