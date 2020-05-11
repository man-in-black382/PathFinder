#ifndef _DenoiserReprojection__
#define _DenoiserReprojection__

#include "GBuffer.hlsl"

struct PassData
{
    uint GBufferNormalTextureIndex;
    uint DepthTextureIndex;
    uint CurrentViewDepthTextureIndex;
    uint PreviousViewDepthTextureIndex;
    uint CurrentAccumulationCounterTextureIndex;
    uint PreviousAccumulationCounterTextureIndex;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const int GroupDimensionSize = 16;
static const int MaxAccumulatedFrames = 32;
// Since we're writing to 8bit unorm texture use normalized increment
static const float FrameCountNormIncrement = 1.0 / float(MaxAccumulatedFrames);
static const float DisocclusionThreshold = 0.01;

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

float ApplyBilinearCustomWeights(float4 v, float4 weights)
{
    // Weight
    v *= weights;
    // Sum
    return dot(v, 1.0);
}

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    uint2 pixelIndex = dispatchThreadID.xy;
    float2 uv = (float2(pixelIndex) + 0.5) / (GlobalDataCB.PipelineRTResolution - 1);

    Texture2D<uint4> gBufferNormalsTexture = UInt4_Textures2D[PassDataCB.GBufferNormalTextureIndex];
    Texture2D depthTexture = Textures2D[PassDataCB.DepthTextureIndex];
    Texture2D previousViewDepthTexture = Textures2D[PassDataCB.PreviousViewDepthTextureIndex];
    Texture2D currentViewDepthTexture = Textures2D[PassDataCB.CurrentViewDepthTextureIndex];
    Texture2D previousAccumulationCounterTexture = Textures2D[PassDataCB.PreviousAccumulationCounterTextureIndex];
    RWTexture2D<float4> currentAccumulationCounterTexture = RW_Float4_Textures2D[PassDataCB.CurrentAccumulationCounterTextureIndex];

    float currentDepth = depthTexture.Load(uint3(pixelIndex, 0)).r;

    float3 surfaceNormal = LoadGBufferNormal(gBufferNormalsTexture, pixelIndex);
    float3 currentSurfacePosition = ReconstructWorldSpacePosition(currentDepth, uv, FrameDataCB.CurrentFrameCamera);
    float3 reprojectedCoord = ViewProjectPoint(currentSurfacePosition, FrameDataCB.PreviousFrameCamera);
    float2 reprojectedUV = (reprojectedCoord.xy + 1.0) * 0.5;
    reprojectedUV.y = 1.0 - reprojectedUV.y;

    Bilinear bilinearFilterAtPrevPos = GetBilinearFilter(saturate(reprojectedUV), GlobalDataCB.PipelineRTResolution);
    
    // Exactly center of 4 texels to get equally weighted values from Gather()
    float2 gatherUV = (bilinearFilterAtPrevPos.Origin + 1.0) * GlobalDataCB.PipelineRTResolutionInv; 
    float4 viewDepthPrev = previousViewDepthTexture.GatherRed(PointClampSampler, gatherUV).wzyx;

    float4 accumCountPrev = previousAccumulationCounterTexture.GatherRed(PointClampSampler, gatherUV).wzyx;
    float4 accumCountNew = min(accumCountPrev + 1.0, MaxAccumulatedFrames);

    // Compute disocclusion
    float4 isInScreen = float4(
        bilinearFilterAtPrevPos.Origin.x >= 0, 
        bilinearFilterAtPrevPos.Origin.x + 1.0 < GlobalDataCB.PipelineRTResolution.x,
        bilinearFilterAtPrevPos.Origin.y >= 0, 
        bilinearFilterAtPrevPos.Origin.y + 1.0 < GlobalDataCB.PipelineRTResolution.y);

    float3 motionVector = float3(0.0, 0.0, 0.0); // TODO: Implement motion vectors

    float3 Xprev = currentSurfacePosition + motionVector;
    float3 Xvprev = mul(FrameDataCB.PreviousFrameCamera.View, float4(Xprev, 1.0)).xyz; 
    float NoXprev = dot(surfaceNormal, Xprev); // Distance to the plane
    float NoVprev = NoXprev / Xvprev.z;
    float4 planeDist = abs(NoVprev * viewDepthPrev - NoXprev); 

    float4 occlusion = step(DisocclusionThreshold, planeDist / NoXprev);
    occlusion = saturate(isInScreen - occlusion);

    float4 weights = GetBilinearCustomWeights(bilinearFilterAtPrevPos, occlusion);
    accumCountNew = ApplyBilinearCustomWeights(accumCountNew, weights);

    currentAccumulationCounterTexture[pixelIndex] = accumCountNew;
}

#endif