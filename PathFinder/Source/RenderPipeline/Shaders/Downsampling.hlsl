#ifndef _AveragingDownscaling__
#define _AveragingDownscaling__

static const uint FilterTypeAverage = 0;
static const uint FilterTypeMin = 1;
static const uint FilterTypeMax = 2;

struct PassData
{
    uint FilterType;
    uint SourceTexIdx; // Full resolution, source
    uint Output0TexIdx; // 1/2 resolution, destination
    uint Output1TexIdx; // 1/4 resolution, destination
    uint Output2TexIdx; // 1/8 resolution, destination
    uint Output3TexIdx; // 1/16 resolution, destination
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const int GroupDimensionSize = 8;
static const int GSArraySize = GroupDimensionSize * GroupDimensionSize;

groupshared float4 gTile[GSArraySize];

float4 Filter(float4 v0, float4 v1, float4 v2, float4 v3)
{
    [branch] switch (PassDataCB.FilterType)
    {
    case FilterTypeMin:     return min(v0, min(v1, min(v2, v3)));   break;
    case FilterTypeMax:     return max(v0, max(v1, max(v2, v3)));   break;
    case FilterTypeAverage: 
    default:                return 0.25 * (v0 + v1 + v2 + v3);      break;
    }
}

// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/DownsampleBloomAllCS.hlsl
//
[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(uint GI : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID)
{
    // Should dispatch for 1/2 resolution 
    Texture2D source = Textures2D[PassDataCB.SourceTexIdx];
    RWTexture2D<float4> destination0 = RW_Float4_Textures2D[PassDataCB.Output0TexIdx];
    RWTexture2D<float4> destination1 = RW_Float4_Textures2D[PassDataCB.Output1TexIdx];
    RWTexture2D<float4> destination2 = RW_Float4_Textures2D[PassDataCB.Output2TexIdx];
    RWTexture2D<float4> destination3 = RW_Float4_Textures2D[PassDataCB.Output3TexIdx];

    uint2 sourceCoord = DTid.xy * 2;

    // You can tell if both x and y are divisible by a power of two with this value
    uint parity = DTid.x | DTid.y;

    // Downsample and store the 8x8 block
    float4 color0 = source[sourceCoord];
    float4 color1 = source[sourceCoord + uint2(1, 0)];
    float4 color2 = source[sourceCoord + uint2(1, 1)];
    float4 color3 = source[sourceCoord + uint2(0, 1)];

    float4 filteredPixel = Filter(color0, color1, color2, color3);

    gTile[GI] = filteredPixel;
    destination0[DTid.xy] = filteredPixel;

    GroupMemoryBarrierWithGroupSync();

    // Downsample and store the 4x4 block
    if ((parity & 1) == 0)
    {
        filteredPixel = Filter(filteredPixel, gTile[GI + 1], gTile[GI + 8], gTile[GI + 9]);
        gTile[GI] = filteredPixel;
        destination1[DTid.xy >> 1] = filteredPixel;
    }

    GroupMemoryBarrierWithGroupSync();

    // Downsample and store the 2x2 block
    if ((parity & 3) == 0)
    {
        filteredPixel = Filter(filteredPixel, gTile[GI + 2], gTile[GI + 16], gTile[GI + 18]);
        gTile[GI] = filteredPixel;
        destination2[DTid.xy >> 2] = filteredPixel;
    }

    GroupMemoryBarrierWithGroupSync();

    // Downsample and store the 1x1 block
    if ((parity & 7) == 0)
    {
        filteredPixel = Filter(filteredPixel, gTile[GI + 4], gTile[GI + 32], gTile[GI + 36]);
        destination3[DTid.xy >> 3] = filteredPixel;
    }
}

#endif