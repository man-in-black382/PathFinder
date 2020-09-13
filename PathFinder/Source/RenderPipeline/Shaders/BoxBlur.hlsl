#ifndef _BoxBlur__
#define _BoxBlur__

#include "GroupsharedMemoryHelpers.hlsl"
#include "ThreadGroupTilingX.hlsl"

struct PassData
{
    uint2 DispatchGroupCount;
    uint InputTexIdx;
    uint OutputTexIdx;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const int GroupDimensionSize = 16;
static const int SamplingRadius = 2;
static const int SamplesInOneDimension = SamplingRadius * 2 + 1;
static const float TotalSampleCountInv = 1.0 / (SamplesInOneDimension * SamplesInOneDimension);
static const int GSArrayDimensionSize = GroupDimensionSize + 2 * SamplingRadius;

groupshared float3 gCache[GSArrayDimensionSize][GSArrayDimensionSize];

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(uint3 groupThreadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
    Texture2D inputTexture = Textures2D[PassDataCB.InputTexIdx];
    RWTexture2D<float4> outputTexture = RW_Float4_Textures2D[PassDataCB.OutputTexIdx];

    uint2 tiledDTId = ThreadGroupTilingX(PassDataCB.DispatchGroupCount, GroupDimensionSize.xx, 16, groupThreadID.xy, groupID.xy);

    GSBoxLoadStoreCoords coords = GetGSBoxLoadStoreCoords(tiledDTId, groupThreadID.xy, GlobalDataCB.PipelineRTResolution, GroupDimensionSize, SamplingRadius);
    gCache[coords.StoreCoord0.x][coords.StoreCoord0.y] = inputTexture[coords.LoadCoord0].rgb;

    if (coords.IsLoadStore1Required) gCache[coords.StoreCoord1.x][coords.StoreCoord1.y] = inputTexture[coords.LoadCoord1].rgb;
    if (coords.IsLoadStore2Required) gCache[coords.StoreCoord2.x][coords.StoreCoord2.y] = inputTexture[coords.LoadCoord2].rgb;
    if (coords.IsLoadStore3Required) gCache[coords.StoreCoord3.x][coords.StoreCoord3.y] = inputTexture[coords.LoadCoord3].rgb;

    GroupMemoryBarrierWithGroupSync();

    int2 groupThreadIndex = groupThreadID.xy + SamplingRadius;
    
    float3 finalColor = 0.0;

    [unroll] for (int x = -SamplingRadius; x <= SamplingRadius; ++x)
    {
        [unroll] for (int y = -SamplingRadius; y <= SamplingRadius; ++y)
        {
            int2 loadCoord = groupThreadIndex + int2(x, y);
            float3 color = gCache[loadCoord.x][loadCoord.y];
            finalColor += color;
        }
    }

    finalColor *= TotalSampleCountInv;

    outputTexture[tiledDTId].rgb = finalColor;// finalColor;
}

#endif