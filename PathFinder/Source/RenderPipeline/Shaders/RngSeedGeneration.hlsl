#ifndef _RngSeedGeneration__
#define _RngSeedGeneration__

struct PassData
{
    uint RngSeedTexIdx;
    uint FrameIdx;
    uint BlueNoiseTexSize;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const int GroupDimensionSize = 16;

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    uint2 pixelIndex = dispatchThreadID.xy;
    RWTexture2D<float4> rngSeedTexture = RW_Float4_Textures2D[PassDataCB.RngSeedTexIdx];

    uint4 rngSeed = uint4(
        pixelIndex.x % PassDataCB.BlueNoiseTexSize,
        pixelIndex.y % PassDataCB.BlueNoiseTexSize,
        PassDataCB.FrameIdx % PassDataCB.BlueNoiseTexSize,
        0);

    rngSeedTexture[pixelIndex] = rngSeed;
}

#endif