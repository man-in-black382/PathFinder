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
    float3 color = source.Sample(PointClampSampler(), pin.UV).rgb;
    return float4(color, 1.0);
}

#endif