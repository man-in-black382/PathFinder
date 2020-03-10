#ifndef _BloomDownscaling__
#define _BloomDownscaling__

struct PassData
{
    float2 SourceInverseDimensions;
    uint SourceTextureIndex;
    uint HalfSizeDestinationTextureIndex;
    uint QuadSizeDestinationTextureIndex;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

groupshared float3 gTiles[64];

[numthreads(8, 8, 1)]
void CSMain(uint groupIndex : SV_GroupIndex, uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D bloomInputTexture = Textures2D[PassDataCB.SourceTextureIndex];
    RWTexture2D<float4> halfSizeOutputImage = RW_Float4_Textures2D[PassDataCB.HalfSizeDestinationTextureIndex];
    RWTexture2D<float4> quadSizeOutputImage = RW_Float4_Textures2D[PassDataCB.QuadSizeDestinationTextureIndex];

    // You can tell if both x and y are divisible by a power of two with this value
    uint parity = dispatchThreadID.x | dispatchThreadID.y;

    float2 centerUV = (float2(dispatchThreadID.xy) * 2.0f + 1.0f) * PassDataCB.SourceInverseDimensions;
    float3 avgPixel = bloomInputTexture.SampleLevel(LinearClampSampler, centerUV, 0.0f).rgb;

    gTiles[groupIndex] = avgPixel;
    halfSizeOutputImage[dispatchThreadID.xy].rgb = avgPixel;

   /* GroupMemoryBarrierWithGroupSync();

    if ((parity & 1) == 0)
    {
        avgPixel = 0.25f * (avgPixel + gTiles[groupIndex + 1] + gTiles[groupIndex + 8] + gTiles[groupIndex + 9]);
        gTiles[groupIndex] = avgPixel;
        quadSizeOutputImage[dispatchThreadID.xy >> 1].rgb = avgPixel;
    }*/
}

#endif