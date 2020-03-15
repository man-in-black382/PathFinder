#ifndef _BloomDownscaling__
#define _BloomDownscaling__

struct PassData
{
    uint FullResSourceTextureIndex;
    uint HalfResDestinationTextureIndex;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

// Should be dispatched with dimensions of destination texture
[numthreads(8, 8, 1)]
void CSMain(uint groupIndex : SV_GroupIndex, uint3 dispatchThreadID : SV_DispatchThreadID)
{
    RWTexture2D<float4> source = RW_Float4_Textures2D[PassDataCB.FullResSourceTextureIndex];
    RWTexture2D<float4> destination = RW_Float4_Textures2D[PassDataCB.HalfResDestinationTextureIndex];

    uint2 sourceCoord = dispatchThreadID.xy * 2;

    float3 color0 = source[sourceCoord].rgb;
    float3 color1 = source[sourceCoord + uint2(1, 0)].rgb;
    float3 color2 = source[sourceCoord + uint2(1, 1)].rgb;
    float3 color3 = source[sourceCoord + uint2(0, 1)].rgb;

    float3 avgColor = (color0 + color1 + color2 + color3) * 0.25;

    destination[dispatchThreadID.xy].rgb = avgColor;
}

#endif