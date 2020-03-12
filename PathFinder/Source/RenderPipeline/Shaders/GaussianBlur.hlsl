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
    uint IsHorizontal;
    uint BlurRadius;
};

// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/sm5-attributes-numthreads

// Groupshared memory is limited to 16KB per group.
// A single thread is limited to a 256 byte region of groupshared memory for writing.

groupshared float3 gCache[GaussianBlurGroupSharedBufferSize]; // Around 5KB

float3 BlurGaussian(int2 dispatchThreadID, int2 groupThreadID, RWTexture2D<float4> source, GaussianBlurParameters parameters)
{
    int radius = int(parameters.BlurRadius);

    // We always use groupThread's X coordinate regardless of blur directionality
    // to be able to get away with one blur function definition and avoid code duplication

    // Gather pixels needed for the (Radius) leftmost threads in the group
    // Clamp against image border if necessary
    if (groupThreadID.x < radius)
    {
        int2 loadCoord = parameters.IsHorizontal ?
            int2(max(dispatchThreadID.x - radius, 0), dispatchThreadID.y) :
            int2(dispatchThreadID.x, max(dispatchThreadID.y - radius, 0));
        
        gCache[groupThreadID.x] = source[loadCoord].rgb;
    }

    // Gather pixels needed for the (Radius) rightmost threads in the group
    // Clamp against image border if necessary
    if (groupThreadID.x >= (GaussianBlurGroupSize - radius))
    {
        int2 loadCoord = parameters.IsHorizontal ?
            int2(max(dispatchThreadID.x + radius, 0), dispatchThreadID.y) :
            int2(dispatchThreadID.x, max(dispatchThreadID.y + radius, 0));

        gCache[groupThreadID.x + 2 * radius] = source[loadCoord].rgb;
    }

    // Gather pixels for threads in the middle of the group
    // Clamp for the case when GroupSize is not multiple of source image dimension
    int2 loadCoord = parameters.IsHorizontal ?
        int2(min(dispatchThreadID.x, parameters.ImageSize.x - 1), dispatchThreadID.y) :
        int2(dispatchThreadID.x, min(dispatchThreadID.y, parameters.ImageSize.y - 1));

    gCache[groupThreadID.x + radius] = source[loadCoord].rgb;

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
        color += gCache[i + radius + groupThreadID.x] * weight;
    }

    return color;
}



//[numthreads(GroupSize, 1, 1)]
//void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
//{
//    Texture2D source = Textures2D[PassDataCB.InputTextureIndex];
//    RWTexture2D<float4> destination = RW_Float4_Textures2D[PassDataCB.OutputTextureIndex];
//
//    int radius = int(PassDataCB.BlurRadius);
//    uint2 boundaries = GlobalDataCB.PipelineRTResolution;
//
//    // Gather pixels needed for the (Radius) leftmost threads in the group
//    // Clamp against image border if necessary
//    if (groupThreadID.x < radius)
//    {
//        int x = max(dispatchThreadID.x - radius, 0);
//        gCache[groupThreadID.x] = source[int2(x, dispatchThreadID.y)].rgb;
//    }
//    
//    // Gather pixels needed for the (Radius) rightmost threads in the group
//    // Clamp against image border if necessary
//    if (groupThreadID.x >= (GroupSize - radius))
//    {
//        int x = min(dispatchThreadID.x + radius, boundaries.x - 1);
//        gCache[groupThreadID.x + 2 * radius] = source[int2(x, dispatchThreadID.y)].rgb;
//    }
//    
//    // Gather pixels for threads in the middle of the group
//    // Clamp for the case when GroupSize is not multiple of source image dimension
//    int2 xy = int2(min(dispatchThreadID.x, boundaries.x - 1), dispatchThreadID.y);
//    gCache[groupThreadID.x + radius] = source[xy].rgb;
//
//    // Wait until every thread in the group finishes writing to groupshared memory
//    GroupMemoryBarrierWithGroupSync();
//
//    // Blur using cached data
//    //
//    float3 color = float3(0.0, 0.0, 0.0);
//    
//    for (int i = -radius; i <= radius; i++)
//    {
//        uint index = uint(abs(i));
//        uint vectorIndex = index / 4;
//        uint elementIndex = index % 4;
//        float4 weightVector = PassDataCB.Weights[vectorIndex];
//        float weight = weightVector[elementIndex];
//        color += gCache[i + radius + groupThreadID.x] * weight;
//    }
//
//    destination[dispatchThreadID.xy] = float4(color, 1.0);
//}

#endif