#ifndef _DenoiserReprojection__
#define _DenoiserReprojection__

#include "DenoiserCommon.hlsl"
#include "GBuffer.hlsl"
#include "Utils.hlsl"
#include "Filtering.hlsl"

struct PassData
{
    uint GBufferNormalRoughnessTexIdx;
    uint DepthTexIdx;
    uint CurrentViewDepthTexIdx;
    uint PreviousViewDepthTexIdx;
    uint CurrentAccumulationCounterTexIdx;
    uint PreviousAccumulationCounterTexIdx;
    uint ShadowedShadingHistoryTexIdx;
    uint UnshadowedShadingHistoryTexIdx;
    uint ShadowedShadingReprojectionTargetTexIdx;
    uint UnshadowedShadingReprojectionTargetTexIdx;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const int GroupDimensionSize = 16;
static const float DisocclusionThreshold = 0.01;

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    //------------------------------------//
    // Ghosting free history reprojection //
    //------------------------------------//

    uint2 pixelIndex = dispatchThreadID.xy;
    float2 uv = (float2(pixelIndex) + 0.5) * GlobalDataCB.PipelineRTResolutionInv; 

    Texture2D normalRoughnessTexture = Textures2D[PassDataCB.GBufferNormalRoughnessTexIdx];
    Texture2D depthTexture = Textures2D[PassDataCB.DepthTexIdx];
    Texture2D prevViewDepthTexture = Textures2D[PassDataCB.PreviousViewDepthTexIdx];
    Texture2D currentViewDepthTexture = Textures2D[PassDataCB.CurrentViewDepthTexIdx];
    Texture2D prevAccumulationCounterTexture = Textures2D[PassDataCB.PreviousAccumulationCounterTexIdx];
    Texture2D shadowedShadingHistoryTexture = Textures2D[PassDataCB.ShadowedShadingHistoryTexIdx];
    Texture2D unshadowedShadingHistoryTexture = Textures2D[PassDataCB.UnshadowedShadingHistoryTexIdx];
    
    RWTexture2D<float4> currentAccumulationCounterTexture = RW_Float4_Textures2D[PassDataCB.CurrentAccumulationCounterTexIdx];
    RWTexture2D<float4> shadowedShadingReprojectionTarget = RW_Float4_Textures2D[PassDataCB.ShadowedShadingReprojectionTargetTexIdx];
    RWTexture2D<float4> unshadowedShadingReprojectionTarget = RW_Float4_Textures2D[PassDataCB.UnshadowedShadingReprojectionTargetTexIdx];

    float currentDepth = depthTexture.Load(uint3(pixelIndex, 0)).r;
    float4 normalRoughness = normalRoughnessTexture.Load(uint3(pixelIndex, 0)).xyzw;
    float3 surfaceNormal = normalRoughness.xyz;
    float3 currentPosition = ReconstructWorldSpacePosition(currentDepth, uv, FrameDataCB.CurrentFrameCamera);
    float3 motionVector = float3(0.0, 0.0, 0.0); // TODO: Implement motion vectors
    float3 previousPosition = currentPosition - motionVector;
    float3 reprojectedCoord = ViewProjectPoint(previousPosition, FrameDataCB.PreviousFrameCamera); 
    float2 reprojectedUV = NDCToUV(reprojectedCoord);

    // Custom binary weights based on disocclusion help us get rid of ghosting
    Bilinear bilinearFilterAtPrevPos = GetBilinearFilter(reprojectedUV, GlobalDataCB.PipelineRTResolutionInv, GlobalDataCB.PipelineRTResolution);

    float4 viewDepthPrev = GatherRedManually(prevViewDepthTexture, bilinearFilterAtPrevPos, PointClampSampler);

    for (int idx = 0; idx < 4; ++idx)
    {
        if (viewDepthPrev[idx] <= 0.0)
        {
            viewDepthPrev[idx] = 10000.0;
        }
    }

    // Compute disocclusion
    float4 isInScreen = float4(
        bilinearFilterAtPrevPos.TopLeftUV.x >= 0.0,
        bilinearFilterAtPrevPos.TopLeftUV.y + bilinearFilterAtPrevPos.TexelSize.y < 1.0,
        bilinearFilterAtPrevPos.TopLeftUV.y >= 0.0,
        bilinearFilterAtPrevPos.TopLeftUV.x + bilinearFilterAtPrevPos.TexelSize.x < 1.0);

    float3 previousViewPosition = mul(FrameDataCB.PreviousFrameCamera.View, float4(previousPosition, 1.0)).xyz; 

    // Absolute plane displacement along normal. 
    float NoXprev = abs(dot(surfaceNormal, previousPosition)); 

    // Distance to plane for each sampled view depth
    float NoVprev = NoXprev / previousViewPosition.z;
    float4 planeDist = abs(NoVprev * viewDepthPrev - NoXprev);  

    // Relative distance determines occlusion
    float4 occlusion = step(DisocclusionThreshold, planeDist / NoXprev);

    // Disocclude off screen pixels
    occlusion = saturate(isInScreen - occlusion);

    float4 weights = GetBilinearCustomWeights(bilinearFilterAtPrevPos, occlusion);

    float4 accumCountPrev = GatherRedManually(prevAccumulationCounterTexture, bilinearFilterAtPrevPos, PointClampSampler);
    float accumCountNew = ApplyBilinearCustomWeights(min(accumCountPrev + 1.0, MaxAccumulatedFrames), weights);

    // Force an aggressive counter reset when 
    // at least one gathered texel was disoccluded
    if (any(occlusion <= 0.0))
    {
        accumCountNew = 0.0; 
    }

    GatheredRGB shadowedShadingGatherResult = GatherRGBManually(shadowedShadingHistoryTexture, bilinearFilterAtPrevPos, PointClampSampler);
    GatheredRGB unshadowedShadingGatherResult = GatherRGBManually(unshadowedShadingHistoryTexture, bilinearFilterAtPrevPos, PointClampSampler);

    float3 shadowedShadingReprojected = float3(
        ApplyBilinearCustomWeights(shadowedShadingGatherResult.Red, weights),
        ApplyBilinearCustomWeights(shadowedShadingGatherResult.Green, weights),
        ApplyBilinearCustomWeights(shadowedShadingGatherResult.Blue, weights));

    float3 unshadowedShadingReprojected = float3(
        ApplyBilinearCustomWeights(unshadowedShadingGatherResult.Red, weights),
        ApplyBilinearCustomWeights(unshadowedShadingGatherResult.Green, weights),
        ApplyBilinearCustomWeights(unshadowedShadingGatherResult.Blue, weights));

    currentAccumulationCounterTexture[pixelIndex] = accumCountNew;
    shadowedShadingReprojectionTarget[pixelIndex].rgb = shadowedShadingReprojected;
    unshadowedShadingReprojectionTarget[pixelIndex].rgb = unshadowedShadingReprojected;
}

#endif