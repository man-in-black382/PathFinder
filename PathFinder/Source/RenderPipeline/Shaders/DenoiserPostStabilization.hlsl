#ifndef _DenoiserHistoryFix__
#define _DenoiserHistoryFix__

#include "GroupsharedMemoryHelpers.hlsl"

struct PassData
{
    uint InputTexIdx;
    uint OutputTexIdx;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"
#include "Random.hlsl"

static const int GroupDimensionSize = 16;
static const int SamplingRadius = 3;
static const int VarianceProbingLineCount = 2;
static const int TotalSampleCount = VarianceProbingLineCount * (SamplingRadius * 2);
static const int SampleCount = SamplingRadius * 2 * SamplingRadius * 2;
static const int GSArrayDimensionSize = GroupDimensionSize + 2 * SamplingRadius;
static const float VarianceClippingGamma = 1;

groupshared float3 gCache[GSArrayDimensionSize][GSArrayDimensionSize];

float3 ClipVariance(int2 groupThreadIndex)
{
    // Variance clipping
    // https://community.arm.com/developer/tools-software/graphics/b/blog/posts/temporal-anti-aliasing
    // https://developer.download.nvidia.com/gameworks/events/GDC2016/msalvi_temporal_supersampling.pdf
    
    groupThreadIndex += SamplingRadius;

    // https://en.wikipedia.org/wiki/Moment_(mathematics)
    //
    float3 M1 = 0; // First moment - Average
    float3 M2 = 0; // Second moment - Variance

    [unroll] for (int lineIdx = 0; lineIdx < VarianceProbingLineCount; ++lineIdx) 
    {
        float angle = Random(lineIdx + groupThreadIndex);
        float s, c;
        sincos(angle, s, c);
        float2 randomDirection = float2(s, c);

        [unroll] for (int sampleIdx = -SamplingRadius; sampleIdx <= SamplingRadius; ++sampleIdx)
        {
            // Skip center
            if (sampleIdx == 0) continue;

            int2 loadCoord = groupThreadIndex + int2(sampleIdx * randomDirection);
            float3 color = gCache[loadCoord.x][loadCoord.y];
            M1 += color;
            M2 += color * color;
        }
    }

    float3 MU = M1 / TotalSampleCount;
    float3 sigma = sqrt(max(M2 / TotalSampleCount - MU * MU, 0.0));

    float3 boxMin = MU - VarianceClippingGamma * sigma;
    float3 boxMax = MU + VarianceClippingGamma * sigma;

    float3 center = gCache[groupThreadIndex.x][groupThreadIndex.y];

    return boxMax;
}

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 DTid : SV_DispatchThreadID, int3 GTid : SV_GroupThreadID)
{
    Texture2D inputTexture = Textures2D[PassDataCB.InputTexIdx];
    RWTexture2D<float4> outputTexture = RW_Float4_Textures2D[PassDataCB.OutputTexIdx];

    GSBoxLoadStoreCoords coords = GetGSBoxLoadStoreCoords(DTid.xy, GTid.xy, GlobalDataCB.PipelineRTResolution, GroupDimensionSize, SamplingRadius);
    gCache[coords.StoreCoord0.x][coords.StoreCoord0.y] = inputTexture[coords.LoadCoord0].rgb;

    if (coords.IsLoadStore1Required) gCache[coords.StoreCoord1.x][coords.StoreCoord1.y] = inputTexture[coords.LoadCoord1].rgb;
    if (coords.IsLoadStore2Required) gCache[coords.StoreCoord2.x][coords.StoreCoord2.y] = inputTexture[coords.LoadCoord2].rgb;
    if (coords.IsLoadStore3Required) gCache[coords.StoreCoord3.x][coords.StoreCoord3.y] = inputTexture[coords.LoadCoord3].rgb;

    GroupMemoryBarrierWithGroupSync();

    outputTexture[DTid.xy].rgb = ClipVariance(GTid.xy);
}

#endif