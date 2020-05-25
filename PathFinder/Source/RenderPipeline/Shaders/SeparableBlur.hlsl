#ifndef _SeparableBlur__
#define _SeparableBlur__

static const int BlurMaximumRadius = 64;
static const int WeightCount = 64;
static const int BlurGroupSize = 256;
static const int BlurGroupSharedBufferSize = BlurGroupSize + 2 * BlurMaximumRadius;

struct PassCBData
{
    // Packing into 4-component vectors 
    // to satisfy constant buffer alignment rules
    float4 Weights[WeightCount / 4];
    float2 ImageSize;
    uint IsHorizontal;
    uint BlurRadius;
    uint InputTexIdx;
    uint OutputTexIdx;
};

#define PassDataType PassCBData

#include "MandatoryEntryPointInclude.hlsl"

groupshared float3 gCache[BlurGroupSharedBufferSize]; // Around 5KB

[numthreads(BlurGroupSize, 1, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    RWTexture2D<float4> source = RW_Float4_Textures2D[PassDataCB.InputTexIdx];
    RWTexture2D<float4> destination = RW_Float4_Textures2D[PassDataCB.OutputTexIdx];

    int2 texelIndex = PassDataCB.IsHorizontal ? dispatchThreadID.xy : dispatchThreadID.yx;
    int groupThreadIndex = groupThreadID.x;
    int radius = int(PassDataCB.BlurRadius);

    // Gather pixels needed for the (Radius) leftmost threads in the group
    // Clamp against image border if necessary
    if (groupThreadIndex < radius)
    {
        int2 loadCoord = PassDataCB.IsHorizontal ?
            int2(max(texelIndex.x - radius, 0), texelIndex.y) :
            int2(texelIndex.x, max(texelIndex.y - radius, 0));

        gCache[groupThreadIndex] = source[loadCoord].rgb;
    }

    // Gather pixels needed for the (Radius) rightmost threads in the group
    // Clamp against image border if necessary
    if (groupThreadIndex >= (BlurGroupSize - radius))
    {
        int2 loadCoord = PassDataCB.IsHorizontal ?
            int2(min(texelIndex.x + radius, PassDataCB.ImageSize.x - 1), texelIndex.y) :
            int2(texelIndex.x, min(texelIndex.y + radius, PassDataCB.ImageSize.y - 1));

        gCache[groupThreadIndex + 2 * radius] = source[loadCoord].rgb;
    }

    // Gather pixels for threads in the middle of the group
    // Clamp for the case when GroupSize is not multiple of source image dimension
    int2 loadCoord = min(texelIndex, PassDataCB.ImageSize - 1);

    gCache[groupThreadIndex + radius] = source[loadCoord].rgb;

    // Wait until every thread in the group finishes writing to groupshared memory
    GroupMemoryBarrierWithGroupSync();

    // Blur using cached data
    //
    float3 color = float3(0.0, 0.0, 0.0);

    for (int i = -radius; i <= radius; i++)
    {
        uint index = uint(abs(i));
        uint vectorIndex = index / 4;
        uint elementIndex = index % 4;
        float4 weightVector = PassDataCB.Weights[vectorIndex];
        float weight = weightVector[elementIndex];
        color += gCache[i + radius + groupThreadIndex] * weight;
    }

    destination[texelIndex] = float4(color, 1.0);
}

#endif