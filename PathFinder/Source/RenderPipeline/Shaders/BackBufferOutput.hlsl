#ifndef _BackBufferOutput__
#define _BackBufferOutput__

struct PassData
{
    uint SourceTextureIndex;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"
#include "FullScreenQuadVS.hlsl"

float4 PSMain(VertexOut pin) : SV_Target
{
    Texture2D source = Textures2D[PassDataCB.SourceTextureIndex];
    float3 color = source.Sample(AnisotropicClampSampler, float3(pin.UV, 0.0)).rgb;
    return float4(color, 1.0);
}

#endif