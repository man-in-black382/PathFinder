#ifndef _DenoiserReprojection__
#define _DenoiserReprojection__

#include "DenoiserCommon.hlsl"
#include "GBuffer.hlsl"
#include "Utils.hlsl"
#include "Filtering.hlsl"
#include "ColorConversion.hlsl"
#include "ThreadGroupTilingX.hlsl"

struct PassData
{
    uint2 DispatchGroupCount;
    uint GBufferNormalRoughnessTexIdx;
    uint DepthTexIdx;
    uint MotionTexIdx;
    uint CurrentViewDepthTexIdx;
    uint PreviousViewDepthTexIdx;
    uint CurrentAccumulationCounterTexIdx;
    uint PreviousAccumulationCounterTexIdx;
    uint ShadowedShadingTexIdx;
    uint UnshadowedShadingTexIdx;
    uint ShadowedShadingHistoryTexIdx;
    uint UnshadowedShadingHistoryTexIdx;
    uint ShadowedShadingReprojectionTargetTexIdx;
    uint UnshadowedShadingReprojectionTargetTexIdx;
    uint ShadingGradientTexIdx;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const int GroupDimensionSize = 16;
static const float DisocclusionThreshold = 0.01;

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(uint3 groupThreadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
    //------------------------------------//
    // Ghosting free history reprojection //
    //------------------------------------//

    uint2 pixelIndex = ThreadGroupTilingX(PassDataCB.DispatchGroupCount, GroupDimensionSize.xx, 16, groupThreadID.xy, groupID.xy);
    float2 uv = TexelIndexToUV(pixelIndex, GlobalDataCB.PipelineRTResolution);

    Texture2D normalRoughnessTexture = Textures2D[PassDataCB.GBufferNormalRoughnessTexIdx];
    Texture2D depthTexture = Textures2D[PassDataCB.DepthTexIdx];
    Texture2D<uint4> motionTexture = UInt4_Textures2D[PassDataCB.MotionTexIdx];
    Texture2D prevViewDepthTexture = Textures2D[PassDataCB.PreviousViewDepthTexIdx];
    Texture2D currentViewDepthTexture = Textures2D[PassDataCB.CurrentViewDepthTexIdx];
    Texture2D prevAccumulationCounterTexture = Textures2D[PassDataCB.PreviousAccumulationCounterTexIdx];
    Texture2D shadowedShadingTexture = Textures2D[PassDataCB.ShadowedShadingTexIdx];
    Texture2D unshadowedShadingTexture = Textures2D[PassDataCB.UnshadowedShadingTexIdx];
    Texture2D shadowedShadingHistoryTexture = Textures2D[PassDataCB.ShadowedShadingHistoryTexIdx];
    Texture2D unshadowedShadingHistoryTexture = Textures2D[PassDataCB.UnshadowedShadingHistoryTexIdx];
    
    RWTexture2D<float4> currentAccumulationCounterTexture = RW_Float4_Textures2D[PassDataCB.CurrentAccumulationCounterTexIdx];
    RWTexture2D<float4> shadowedShadingReprojectionTarget = RW_Float4_Textures2D[PassDataCB.ShadowedShadingReprojectionTargetTexIdx];
    RWTexture2D<float4> unshadowedShadingReprojectionTarget = RW_Float4_Textures2D[PassDataCB.UnshadowedShadingReprojectionTargetTexIdx];
    RWTexture2D<float4> shadingGradientTarget = RW_Float4_Textures2D[PassDataCB.ShadingGradientTexIdx];

    float roughness;
    float3 surfaceNormal;
    LoadGBufferNormalAndRoughness(normalRoughnessTexture, pixelIndex, surfaceNormal, roughness);

    float currentDepth = depthTexture.Load(uint3(pixelIndex, 0)).r;
    float3 currentPosition = NDCDepthToWorldPosition(currentDepth, uv, FrameDataCB.CurrentFrameCamera);
    float3 motionVector = LoadGBufferMotion(motionTexture, pixelIndex);
    float3 previousPosition = currentPosition - motionVector;
    float3 reprojectedCoord = ViewProject(previousPosition, FrameDataCB.PreviousFrameCamera); 
    float2 reprojectedUV = NDCToUV(reprojectedCoord);

    // Custom binary weights based on disocclusion help us get rid of ghosting
    Bilinear bilinearFilterAtPrevPos = GetBilinearFilter(reprojectedUV, GlobalDataCB.PipelineRTResolutionInv, GlobalDataCB.PipelineRTResolution);

    float4 viewDepthPrev = GatherRedManually(prevViewDepthTexture, bilinearFilterAtPrevPos, 0.0, PointClampSampler());

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

    float4 accumCountPrev = GatherRedManually(prevAccumulationCounterTexture, bilinearFilterAtPrevPos, 0.0, PointClampSampler());
    float accumCountNew = ApplyBilinearCustomWeights(min(accumCountPrev + 1.0, MaxAccumulatedFrames), weights);

    // Force an aggressive counter reset when 
    // at least one gathered texel was disoccluded
    if (any(occlusion <= 0.0))
    {
        accumCountNew = 0.0; 
    }

    GatheredRGB shadowedShadingGatherResult = GatherRGBManually(shadowedShadingHistoryTexture, bilinearFilterAtPrevPos, 0.0, PointClampSampler());
    GatheredRGB unshadowedShadingGatherResult = GatherRGBManually(unshadowedShadingHistoryTexture, bilinearFilterAtPrevPos, 0.0, PointClampSampler());

    float3 shadowedShadingReprojected = float3(
        ApplyBilinearCustomWeights(shadowedShadingGatherResult.Red, weights),
        ApplyBilinearCustomWeights(shadowedShadingGatherResult.Green, weights),
        ApplyBilinearCustomWeights(shadowedShadingGatherResult.Blue, weights));

    float3 unshadowedShadingReprojected = float3(
        ApplyBilinearCustomWeights(unshadowedShadingGatherResult.Red, weights),
        ApplyBilinearCustomWeights(unshadowedShadingGatherResult.Green, weights),
        ApplyBilinearCustomWeights(unshadowedShadingGatherResult.Blue, weights));

    shadowedShadingReprojectionTarget[pixelIndex].rgb = shadowedShadingReprojected;
    unshadowedShadingReprojectionTarget[pixelIndex].rgb = unshadowedShadingReprojected;

    currentAccumulationCounterTexture[pixelIndex] = accumCountNew;
}

#endif