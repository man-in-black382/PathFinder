#ifndef _AveragingDownscaling__
#define _AveragingDownscaling__

struct PassData
{
    uint SourceTextureIndex; // Full resolution, source
    uint Output0TextureIndex; // 1/2 resolution, destination
    uint Output1TextureIndex; // 1/4 resolution, destination
    uint Output2TextureIndex; // 1/8 resolution, destination
    uint Output3TextureIndex; // 1/16 resolution, destination
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const int GroupDimensionSize = 8;
static const int GSArraySize = GroupDimensionSize * GroupDimensionSize;

groupshared float3 gTile[GSArraySize];

// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/DownsampleBloomAllCS.hlsl
//
[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(uint GI : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID)
{
    // TODO: Implement permutation system to use this algorithm with variable mip counts

    // Should dispatch for 1/2 resolution 
    RWTexture2D<float4> source = RW_Float4_Textures2D[PassDataCB.SourceTextureIndex];
    RWTexture2D<float4> destination0 = RW_Float4_Textures2D[PassDataCB.Output0TextureIndex];
    RWTexture2D<float4> destination1 = RW_Float4_Textures2D[PassDataCB.Output1TextureIndex];
    RWTexture2D<float4> destination2 = RW_Float4_Textures2D[PassDataCB.Output2TextureIndex];
    RWTexture2D<float4> destination3 = RW_Float4_Textures2D[PassDataCB.Output3TextureIndex];

    uint2 sourceCoord = DTid.xy * 2;

    // You can tell if both x and y are divisible by a power of two with this value
    uint parity = DTid.x | DTid.y;

    // Downsample and store the 8x8 block

    // TODO: Rework engine to support different states for different mip levels.
    // Right now all mip levels are UAVs, but ideally 0 mip should be SRV to use hardware interpolation.
    float3 color0 = source[sourceCoord].rgb;
    float3 color1 = source[sourceCoord + uint2(1, 0)].rgb;
    float3 color2 = source[sourceCoord + uint2(1, 1)].rgb;
    float3 color3 = source[sourceCoord + uint2(0, 1)].rgb;

    float3 avgPixel = (color0 + color1 + color2 + color3) * 0.25;

    gTile[GI] = avgPixel;
    destination0[DTid.xy].rgb = avgPixel;

    GroupMemoryBarrierWithGroupSync();

    // Downsample and store the 4x4 block
    if ((parity & 1) == 0)
    {
        avgPixel = 0.25f * (avgPixel + gTile[GI + 1] + gTile[GI + 8] + gTile[GI + 9]);
        gTile[GI] = avgPixel;
        destination1[DTid.xy >> 1].rgb = avgPixel;
    }

    GroupMemoryBarrierWithGroupSync();

    // Downsample and store the 2x2 block
    if ((parity & 3) == 0)
    {
        avgPixel = 0.25f * (avgPixel + gTile[GI + 2] + gTile[GI + 16] + gTile[GI + 18]);
        gTile[GI] = avgPixel;
        destination2[DTid.xy >> 2].rgb = avgPixel;
    }

    GroupMemoryBarrierWithGroupSync();

    // Downsample and store the 1x1 block
    if ((parity & 7) == 0)
    {
        avgPixel = 0.25f * (avgPixel + gTile[GI + 4] + gTile[GI + 32] + gTile[GI + 36]);
        destination3[DTid.xy >> 3].rgb = avgPixel;
    }
}

#endif