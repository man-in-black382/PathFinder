#ifndef _DenoiserReprojection__
#define _DenoiserReprojection__

struct PassData
{
    uint CurrentDepthTextureIndex;
    uint PreviousDepthTextureIndex;
    uint CurrentAccumulationCounterTextureIndex;
    uint PreviousAccumulationCounterTextureIndex;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const int GroupDimensionSize = 16;

struct Bilinear
{
    float2 Origin;
    float2 Weights;
};

Bilinear GetBilinearFilter(float2 uv, float2 textureSize)
{
    float2 i = uv * textureSize - 0.5;

    Bilinear result;
    result.Origin = floor(i);
    result.Weights = frac(i);

    return result;
}

float4 GetBilinearCustomWeights(Bilinear f, float4 customWeights)
{
    float4 weights;

    // Expand lerp-s into separate weights
    weights.x = (1.0 - f.Weights.x) * (1.0 - f.Weights.y);
    weights.y = f.Weights.x * (1.0 - f.Weights.y);
    weights.z = (1.0 - f.Weights.x) * f.Weights.y;
    weights.w = f.Weights.x * f.Weights.y;

    // Apply custom weights
    weights *= customWeights;

    // Renormalize
    float sum = dot(weights, 1.0);
    weights /= sum;

    return weights;
}

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    uint2 pixelIndex = dispatchThreadID.xy;
    float2 uv = (float2(pixelIndex) + 0.5) / (GlobalDataCB.PipelineRTResolution - 1);

    Texture2D previousDepthTexture = Textures2D[PassDataCB.PreviousDepthTextureIndex];
    Texture2D currentDepthTexture = Textures2D[PassDataCB.CurrentDepthTextureIndex];

    float previousDepth = previousDepthTexture.Load(uint3(pixelIndex, 0)).r;
    float currentDepth = currentDepthTexture.Load(uint3(pixelIndex, 0)).r;

    float3 currentSurfacePosition = ReconstructWorldSpacePosition(currentDepth, uv, FrameDataCB.CurrentFrameCamera);
    float3 reprojectedCoord = ViewProjectPoint(currentSurfacePosition, FrameDataCB.PreviousFrameCamera);
    float2 reprojectedUV = reprojectedCoord.xy;
    float reprojectedDepth = reprojectedCoord.z;

    Bilinear bilinearFilterAtPrevPos = GetBilinearFilter(saturate(reprojectedUV), GlobalDataCB.PipelineRTResolution);
    float2 gatherUV = (bilinearFilterAtPrevPos.Origin + 1.0) * GlobalDataCB.PipelineRTResolutionInv;
    float4 depthPrev = previousDepthTexture.GatherRed(PointClampSampler, gatherUV).wzyx;
    //float4 accumSpeedPrev =  
}

#endif