#ifndef _AveragingDownscaling__
#define _AveragingDownscaling__

static const uint FilterTypeAverage = 0;
static const uint FilterTypeMin = 1;
static const uint FilterTypeMax = 2;

struct PassData
{
    uint FilterType;
    uint SourceTexIdx;
    bool IsSourceTexSRV;
    uint NumberOfOutputsToCompute;
    uint4 OutputTexIndices;
    bool4 OutputsToWrite;
    uint2 InputSize;
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

float4 Filter(float4 v0, float4 v1)
{
    [branch] switch (PassDataCB.FilterType)
    {
    case FilterTypeMin:     return min(v0, v1);     break;
    case FilterTypeMax:     return max(v0, v1);     break;
    case FilterTypeAverage:
    default:                return 0.5 * (v0 + v1); break;
    }
}

// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/DownsampleBloomAllCS.hlsl
//
[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(uint GI : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID)
{
    // Should dispatch for 1/2 resolution 
    RWTexture2D<float4> destination0 = RW_Float4_Textures2D[PassDataCB.OutputTexIndices[0]];
    RWTexture2D<float4> destination1 = RW_Float4_Textures2D[PassDataCB.OutputTexIndices[1]];
    RWTexture2D<float4> destination2 = RW_Float4_Textures2D[PassDataCB.OutputTexIndices[2]];
    RWTexture2D<float4> destination3 = RW_Float4_Textures2D[PassDataCB.OutputTexIndices[3]];

    uint2 sourceCoord = DTid.xy * 2;

    // You can tell if both x and y are divisible by a power of two with this value
    uint parity = DTid.x | DTid.y;

    // Downsample and store the 8x8 block
    float4 color0, color1, color2, color3;

    if (PassDataCB.IsSourceTexSRV)
    {
        Texture2D source = Textures2D[PassDataCB.SourceTexIdx];
        color0 = source[sourceCoord];
        color1 = source[sourceCoord + uint2(1, 0)];
        color2 = source[sourceCoord + uint2(1, 1)];
        color3 = source[sourceCoord + uint2(0, 1)];
    }
    else
    {
        RWTexture2D<float4> source = RW_Float4_Textures2D[PassDataCB.SourceTexIdx];
        color0 = source[sourceCoord];
        color1 = source[sourceCoord + uint2(1, 0)];
        color2 = source[sourceCoord + uint2(1, 1)];
        color3 = source[sourceCoord + uint2(0, 1)];
    }

    float4 filteredPixel = Filter(color0, color1, color2, color3);

    if (PassDataCB.NumberOfOutputsToCompute < 1)
    {
        return;
    }

    gTile[GI] = filteredPixel;

    if (PassDataCB.OutputsToWrite[0])
    {
        destination0[DTid.xy] = filteredPixel;
    }
    
    GroupMemoryBarrierWithGroupSync();

    if (PassDataCB.NumberOfOutputsToCompute < 2)
    {
        return;
    }

    // Downsample and store the 4x4 block
    if ((parity & 1) == 0)
    {
        filteredPixel = Filter(filteredPixel, gTile[GI + 1], gTile[GI + 8], gTile[GI + 9]);
        gTile[GI] = filteredPixel;

        if (PassDataCB.OutputsToWrite[1])
        {
            destination1[DTid.xy >> 1] = filteredPixel;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (PassDataCB.NumberOfOutputsToCompute < 3)
    {
        return;
    }

    // Downsample and store the 2x2 block
    if ((parity & 3) == 0)
    {
        filteredPixel = Filter(filteredPixel, gTile[GI + 2], gTile[GI + 16], gTile[GI + 18]);
        gTile[GI] = filteredPixel;

        if (PassDataCB.OutputsToWrite[2])
        {
            destination2[DTid.xy >> 2] = filteredPixel;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (PassDataCB.NumberOfOutputsToCompute < 4)
    {
        return;
    }

    // Downsample and store the 1x1 block
    if ((parity & 7) == 0)
    {
        filteredPixel = Filter(filteredPixel, gTile[GI + 4], gTile[GI + 32], gTile[GI + 36]);

        if (PassDataCB.OutputsToWrite[3])
        {
            destination3[DTid.xy >> 3] = filteredPixel;
        }
    }
}

#endif