#ifndef _AveragingDownscaling__
#define _AveragingDownscaling__

static const uint FilterTypeAverage = 0;
static const uint FilterTypeMin = 1;
static const uint FilterTypeMax = 2;

struct PassData
{
    uint FilterType;
    uint SourceTexIdx;
    uint SourceMipIdx;
    uint NumberOfOutputsToCompute;
    uint4 OutputTexIndices;
    bool4 OutputsToWrite;
    float2 DispatchDimensionsInv;
    bool IsInputSizeOddVertically;
    bool IsInputSizeOddHorizontally;
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
    case FilterTypeMin:     return min(v0, min(v1, min(v2, v3)));  
    case FilterTypeMax:     return max(v0, max(v1, max(v2, v3)));  
    case FilterTypeAverage: 
    default:                return 0.25 * (v0 + v1 + v2 + v3);     
    }
}

float4 Filter(float4 v0, float4 v1)
{
    [branch] switch (PassDataCB.FilterType)
    {
    case FilterTypeMin:     return min(v0, v1);    
    case FilterTypeMax:     return max(v0, v1);    
    case FilterTypeAverage:
    default:                return 0.5 * (v0 + v1);
    }
}

sampler GetSampler()
{
    switch (PassDataCB.FilterType)
    {
    case FilterTypeMin:     return MinSampler();
    case FilterTypeMax:     return MaxSampler();
    case FilterTypeAverage:
    default:                return LinearClampSampler();
    }
}

// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/DownsampleBloomAllCS.hlsl
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/GenerateMipsCS.hlsli
//
[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(uint GI : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID)
{
    if (PassDataCB.NumberOfOutputsToCompute < 1)
    {
        return;
    }

    // Should dispatch for 1/2 resolution 
    RWTexture2D<float4> destination0 = RW_Float4_Textures2D[PassDataCB.OutputTexIndices[0]];
    RWTexture2D<float4> destination1 = RW_Float4_Textures2D[PassDataCB.OutputTexIndices[1]];
    RWTexture2D<float4> destination2 = RW_Float4_Textures2D[PassDataCB.OutputTexIndices[2]];
    RWTexture2D<float4> destination3 = RW_Float4_Textures2D[PassDataCB.OutputTexIndices[3]];

    float2 texelSize = PassDataCB.DispatchDimensionsInv;

    // You can tell if both x and y are divisible by a power of two with this value
    uint parity = DTid.x | DTid.y;

    // Downsample and store the 8x8 block
    float4 filteredPixel;
    Texture2D source = Textures2D[PassDataCB.SourceTexIdx];

    // One bilinear sample is insufficient when scaling down by more than 2x.
    // You will slightly undersample in the case where the source dimension
    // is odd.  This is why it's a really good idea to only generate mips on
    // power-of-two sized textures.  Trying to handle the undersampling case
    // will force this shader to be slower and more complicated as it will
    // have to take more source texture samples.

    sampler samplerState = GetSampler();

    if (!PassDataCB.IsInputSizeOddVertically && !PassDataCB.IsInputSizeOddHorizontally)
    {
        float2 UV = texelSize * (DTid.xy + 0.5);
        filteredPixel = source.SampleLevel(samplerState, UV, PassDataCB.SourceMipIdx);
    }
    else if (PassDataCB.IsInputSizeOddHorizontally && PassDataCB.IsInputSizeOddVertically)
    {
        // > 2:1 in in both dimensions
        // Use 4 bilinear samples to guarantee we don't undersample when downsizing by more than 2x
        // in both directions.
        float2 UV1 = texelSize * (DTid.xy + float2(0.25, 0.25));
        float2 O = texelSize * 0.5;
        filteredPixel = Filter(
            source.SampleLevel(samplerState, UV1, PassDataCB.SourceMipIdx),
            source.SampleLevel(samplerState, UV1 + float2(O.x, 0.0), PassDataCB.SourceMipIdx),
            source.SampleLevel(samplerState, UV1 + float2(0.0, O.y), PassDataCB.SourceMipIdx),
            source.SampleLevel(samplerState, UV1 + float2(O.x, O.y), PassDataCB.SourceMipIdx)
        );
    }
    else if (PassDataCB.IsInputSizeOddHorizontally)
    {
        // > 2:1 in X dimension
        // Use 2 bilinear samples to guarantee we don't undersample when downsizing by more than 2x
        // horizontally.
        float2 UV1 = texelSize * (DTid.xy + float2(0.25, 0.5));
        float2 Off = texelSize * float2(0.5, 0.0);
        filteredPixel = Filter(
            source.SampleLevel(samplerState, UV1, PassDataCB.SourceMipIdx),
            source.SampleLevel(samplerState, UV1 + Off, PassDataCB.SourceMipIdx)
        );
    }
    else if (PassDataCB.IsInputSizeOddVertically)
    {
        // > 2:1 in Y dimension
        // Use 2 bilinear samples to guarantee we don't undersample when downsizing by more than 2x
        // vertically.
        float2 UV1 = texelSize * (DTid.xy + float2(0.5, 0.25));
        float2 Off = texelSize * float2(0.0, 0.5);

        filteredPixel = Filter(
            source.SampleLevel(samplerState, UV1, PassDataCB.SourceMipIdx),
            source.SampleLevel(samplerState, UV1 + Off, PassDataCB.SourceMipIdx)
        );
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