#ifndef _GaussianBlur__
#define _GaussianBlur__

static const int GaussianBlurMaximumRadius = 64;
static const int GaussianBlurGroupSize = 256;
static const int GaussianBlurGroupSharedBufferSize = GaussianBlurGroupSize + 2 * GaussianBlurMaximumRadius;

struct GaussianBlurParameters
{
    // Packing into 4-component vectors 
    // to satisfy constant buffer alignment rules
    float4 Weights[GaussianBlurMaximumRadius / 4]; 
    float2 ImageSize;
    bool IsHorizontal;
    uint BlurRadius;
};

// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/sm5-attributes-numthreads

// Groupshared memory is limited to 16KB per group.
// A single thread is limited to a 256 byte region of groupshared memory for writing.

groupshared float3 gCache[GaussianBlurGroupSharedBufferSize]; // Around 5KB

float3 BlurGaussian(int2 dispatchThreadID, int groupThreadIndex, RWTexture2D<float4> source, GaussianBlurParameters parameters)
{
    int radius = int(parameters.BlurRadius);

    // Gather pixels needed for the (Radius) leftmost threads in the group
    // Clamp against image border if necessary
    if (groupThreadIndex < radius)
    {
        int2 loadCoord = parameters.IsHorizontal ?
            int2(max(dispatchThreadID.x - radius, 0), dispatchThreadID.y) :
            int2(dispatchThreadID.x, max(dispatchThreadID.y - radius, 0));
        
        gCache[groupThreadIndex] = source[loadCoord].rgb;
    }

    // Gather pixels needed for the (Radius) rightmost threads in the group
    // Clamp against image border if necessary
    if (groupThreadIndex >= (GaussianBlurGroupSize - radius))
    {
        int2 loadCoord = parameters.IsHorizontal ?
            int2(min(dispatchThreadID.x + radius, parameters.ImageSize.x - 1), dispatchThreadID.y) :
            int2(dispatchThreadID.x, min(dispatchThreadID.y + radius, parameters.ImageSize.y - 1));

        gCache[groupThreadIndex + 2 * radius] = source[loadCoord].rgb;
    }

    // Gather pixels for threads in the middle of the group
    // Clamp for the case when GroupSize is not multiple of source image dimension
    int2 loadCoord = min(dispatchThreadID, parameters.ImageSize - 1);

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
        float4 weightVector = parameters.Weights[vectorIndex];
        float weight = weightVector[elementIndex];
        color += gCache[i + radius + groupThreadIndex] * weight;
    }

    return color;
}

#endif