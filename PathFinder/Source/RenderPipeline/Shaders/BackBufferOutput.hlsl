#ifndef _BackBufferOutput__
#define _BackBufferOutput__

struct PassData
{
    uint SourceTexIdx;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"
#include "FullScreenQuadVS.hlsl"
#include "ColorConversion.hlsl"

float4 PSMain(VertexOut pin) : SV_Target
{
    Texture2D source = Textures2D[PassDataCB.SourceTexIdx];
    float3 color = source.Sample(AnisotropicClampSampler(), float3(pin.UV, 0.0)).rgb;
    return float4(SRGBFromLinear(color), 1.0);
}

#endif