struct PassData
{
    uint SourceTextureIndex;
};

#define PassDataType PassData

#include "BaseRootSignature.hlsl"
#include "FullScreenQuadVS.hlsl"

float4 PSMain(VertexOut pin) : SV_Target
{
    Texture2D source = Textures2D[PassDataCB.SourceTextureIndex];
    return float4(source.Sample(AnisotropicClampSampler, float3(pin.UV, 0.0)).rgb, 1.0);  
}