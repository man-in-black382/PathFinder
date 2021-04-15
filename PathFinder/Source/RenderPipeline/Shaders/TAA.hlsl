#ifndef _DenoiserHistoryFix__
#define _DenoiserHistoryFix__

#include "GroupsharedMemoryHelpers.hlsl"

struct PassData
{
    uint VarianceSourceTexIdx;
    uint InputTexIdx;
    uint OutputTexIdx;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"
#include "Random.hlsl"

static const int GroupDimensionSize = 16;
static const int SamplingRadius = 1;
static const int SampleCount = 9;
static const int GSArrayDimensionSize = GroupDimensionSize + 2 * SamplingRadius;
static const float VarianceClippingGamma = 1;

groupshared float3 gCache[GSArrayDimensionSize][GSArrayDimensionSize];

float3 ClipVariance(int2 groupThreadIndex, float3 valueToClip)
{
    // Variance clipping
    // https://community.arm.com/developer/tools-software/graphics/b/blog/posts/temporal-anti-aliasing
    // https://developer.download.nvidia.com/gameworks/events/GDC2016/msalvi_temporal_supersampling.pdf
    
    groupThreadIndex += SamplingRadius;

    // https://en.wikipedia.org/wiki/Moment_(mathematics)
    //
    float3 center = gCache[groupThreadIndex.x][groupThreadIndex.y];

    float3 M1 = center; // First moment - Average
    float3 M2 = center * center; // Second moment - Variance

    [unroll] for (int x = -SamplingRadius; x <= SamplingRadius; ++x)
    {
        [unroll] for (int y = -SamplingRadius; y <= SamplingRadius; ++y)
        {
            if (x == 0 && y == 0) continue;

            int2 loadCoord = groupThreadIndex + int2(x, y);
            float3 color = gCache[loadCoord.x][loadCoord.y];
            M1 += color;
            M2 += color * color;
        }
    }

    float3 MU = M1 / SampleCount;
    float3 sigma = sqrt(max(M2 / SampleCount - MU * MU, 0.0));

    float3 boxMin = MU - VarianceClippingGamma * sigma;
    float3 boxMax = MU + VarianceClippingGamma * sigma;

    return clamp(valueToClip, boxMin, boxMax);
}

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 DTid : SV_DispatchThreadID, int3 GTid : SV_GroupThreadID)
{
    Texture2D varianceSourceTexture = Textures2D[PassDataCB.VarianceSourceTexIdx];
    Texture2D inputTexture = Textures2D[PassDataCB.InputTexIdx];
    RWTexture2D<float4> outputTexture = RW_Float4_Textures2D[PassDataCB.OutputTexIdx];

    GSBoxLoadStoreCoords coords = GetGSBoxLoadStoreCoords(DTid.xy, GTid.xy, GlobalDataCB.PipelineRTResolution, GroupDimensionSize, SamplingRadius);
    gCache[coords.StoreCoord0.x][coords.StoreCoord0.y] = varianceSourceTexture[coords.LoadCoord0].rgb;

    if (coords.IsLoadStore1Required) gCache[coords.StoreCoord1.x][coords.StoreCoord1.y] = varianceSourceTexture[coords.LoadCoord1].rgb;
    if (coords.IsLoadStore2Required) gCache[coords.StoreCoord2.x][coords.StoreCoord2.y] = varianceSourceTexture[coords.LoadCoord2].rgb;
    if (coords.IsLoadStore3Required) gCache[coords.StoreCoord3.x][coords.StoreCoord3.y] = varianceSourceTexture[coords.LoadCoord3].rgb;

    GroupMemoryBarrierWithGroupSync();

    outputTexture[DTid.xy].rgb = ClipVariance(GTid.xy, inputTexture[DTid.xy].rgb);
    //outputTexture[DTid.xy].rgb = inputTexture[DTid.xy].rgb;
}

#endif