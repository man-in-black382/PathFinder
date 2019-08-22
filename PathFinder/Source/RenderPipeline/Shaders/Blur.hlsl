static const int MaximumRadius = 64;
static const int GroupSize = 256;
static const int GroupSharedBufferSize = GroupSize + 2 * MaximumRadius;

struct BlurPassData
{
    uint BlurRadius;
    float Weights[MaximumRadius + 1];
    uint InputTextureIndex;
    uint OutputTextureIndex;
};

#define PassDataType BlurPassData

#include "BaseRootSignature.hlsl"

// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/sm5-attributes-numthreads

// Groupshared memory is limited to 16KB per group.
// A single thread is limited to a 256 byte region of groupshared memory for writing.

groupshared float3 gCache[GroupSharedBufferSize]; // Around 5KB

[numthreads(GroupSize, 1, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    Texture2D source = Textures2D[PassDataCB.InputTextureIndex];
    RWTexture2D<float4> destination = RWTextures2D[PassDataCB.OutputTextureIndex];

    //int radius = int(PassDataCB.BlurRadius);
    //uint2 boundaries = GlobalDataCB.PipelineRTResolution;

    //// Gather pixels needed for the (Radius) leftmost threads in the group
    //// Clamp against image border if necessary
    //if (groupThreadID.x < radius)
    //{
    //    int x = max(dispatchThreadID.x - radius, 0);
    //    gCache[groupThreadID.x] = source[int2(x, dispatchThreadID.y)].rgb;
    //}
    //
    //// Gather pixels needed for the (Radius) rightmost threads in the group
    //// Clamp against image border if necessary
    //if (groupThreadID.x >= (GroupSize - radius))
    //{
    //    int x = min(dispatchThreadID.x + radius, boundaries.x - 1);
    //    gCache[groupThreadID.x + 2 * radius] = source[int2(x, dispatchThreadID.y)];
    //}
    //
    //// Gather pixels for threads in the middle of the group
    //// Clamp for the case when GroupSize is not multiple of source image dimension
    //int2 xy = int2(min(dispatchThreadID.x, boundaries.x - 1), dispatchThreadID.y);
    //gCache[groupThreadID.x + radius] = source[xy].rgb;

    //// Wait untill every thread in the group finishes writing to groupshared memory
    //GroupMemoryBarrierWithGroupSync();

    //// Blur using cached data
    ////
    //float3 color = float3(0.0, 0.0, 0.0);
    //
    //for (int i = -radius; i <= radius; i++)
    //{
    //    float weight = PassDataCB.Weights[uint(i)];
    //    color += gCache[i + radius + groupThreadID.x] * weight;
    //}

    //destination[dispatchThreadID.xy] = float4(color, 1.0);

    destination[dispatchThreadID.xy] = source[dispatchThreadID.xy];
}