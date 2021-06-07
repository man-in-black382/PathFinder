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
    uint MotionTexIdx;
    uint DepthStencilTexIdx;
    uint PreviousViewDepthTexIdx;
    uint CurrentAccumulationCounterTexIdx;
    uint PreviousAccumulationCounterTexIdx;
    uint ShadowedShadingDenoisedHistoryTexIdx;
    uint UnshadowedShadingDenoisedHistoryTexIdx;
    uint ShadowedShadingReprojectionTargetTexIdx;
    uint UnshadowedShadingReprojectionTargetTexIdx;
    uint ReprojectedTexelIndicesTargetTexIdx;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const int GroupDimensionSize = 8;

//groupshared min16float3 gReprojectionInfo[GroupDimensionSize][GroupDimensionSize];

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(uint3 groupThreadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
    //------------------------------------//
    // Ghosting free history reprojection //
    //------------------------------------//

    // ========================================================================================================================== //
    Texture2D normalRoughnessTexture = Textures2D[PassDataCB.GBufferNormalRoughnessTexIdx];
    Texture2D<uint4> motionTexture = UInt4_Textures2D[PassDataCB.MotionTexIdx];
    Texture2D prevViewDepthTexture = Textures2D[PassDataCB.PreviousViewDepthTexIdx];
    Texture2D depthStencilTexture = Textures2D[PassDataCB.DepthStencilTexIdx];
    Texture2D prevAccumulationCounterTexture = Textures2D[PassDataCB.PreviousAccumulationCounterTexIdx];
    Texture2D shadowedShadingDenoisedHistoryTexture = Textures2D[PassDataCB.ShadowedShadingDenoisedHistoryTexIdx];
    Texture2D unshadowedShadingDenoisedHistoryTexture = Textures2D[PassDataCB.UnshadowedShadingDenoisedHistoryTexIdx];
    
    RWTexture2D<float4> currentAccumulationCounterTexture = RW_Float4_Textures2D[PassDataCB.CurrentAccumulationCounterTexIdx];
    RWTexture2D<float4> shadowedShadingReprojectionTarget = RW_Float4_Textures2D[PassDataCB.ShadowedShadingReprojectionTargetTexIdx];
    RWTexture2D<float4> unshadowedShadingReprojectionTarget = RW_Float4_Textures2D[PassDataCB.UnshadowedShadingReprojectionTargetTexIdx];
    RWTexture2D<float4> reprojectedTexelIndicesTarget = RW_Float4_Textures2D[PassDataCB.ReprojectedTexelIndicesTargetTexIdx];
    // ========================================================================================================================== //

    uint2 pixelIndex = ThreadGroupTilingX(PassDataCB.DispatchGroupCount, GroupDimensionSize.xx, 8, groupThreadID.xy, groupID.xy);
    float2 uv = TexelIndexToUV(pixelIndex, GlobalDataCB.PipelineRTResolution);

    float roughness;
    float3 surfaceNormal;
    LoadGBufferNormalAndRoughness(normalRoughnessTexture, pixelIndex, surfaceNormal, roughness);
    
    float depth = depthStencilTexture[pixelIndex].r;
    float3 motionVector = LoadGBufferMotion(motionTexture, pixelIndex);
    float2 reprojectedUV = uv - motionVector.xy;
    float depthInPrevFrame = depth - motionVector.z;
    float3 previousViewPosition; 
    float3 previousWorldPosition;

    NDCDepthToViewAndWorldPositions(depthInPrevFrame, reprojectedUV, FrameDataCB.PreviousFrameCamera, previousViewPosition, previousWorldPosition);

    // Custom binary weights based on disocclusion help us get rid of ghosting
    Bilinear bilinearFilterAtPrevPos = GetBilinearFilter(reprojectedUV, GlobalDataCB.PipelineRTResolutionInv, GlobalDataCB.PipelineRTResolution);

    float4 viewDepthPrev = GatherRedManually(prevViewDepthTexture, bilinearFilterAtPrevPos, 0.0, PointClampSampler());

    // Compute disocclusion
    float4 isInScreen = float4(
        bilinearFilterAtPrevPos.TopLeftUV.x >= 0.0,
        bilinearFilterAtPrevPos.TopLeftUV.y + bilinearFilterAtPrevPos.TexelSize.y < 1.0,
        bilinearFilterAtPrevPos.TopLeftUV.y >= 0.0,
        bilinearFilterAtPrevPos.TopLeftUV.x + bilinearFilterAtPrevPos.TexelSize.x < 1.0);

    // Relative distance determines occlusion
    float4 occlusion = ReprojectionOcclusion(surfaceNormal, previousWorldPosition, previousViewPosition, viewDepthPrev);

    // Disocclude off screen pixels
    occlusion = saturate(isInScreen - occlusion);

    float4 weights = GetBilinearCustomWeights(bilinearFilterAtPrevPos, occlusion);
    float4 accumCountPrev = GatherRedManually(prevAccumulationCounterTexture, bilinearFilterAtPrevPos, 0.0, PointClampSampler());
    float accumCountNew = ApplyBilinearCustomWeights(min(accumCountPrev + 1.0, MaxAccumulatedFrames), weights);

    // Force an aggressive counter reset when 
    // at least one gathered texel was disoccluded, because it was out of the screen
    if (any(occlusion <= 0.0) && any(!isInScreen))
    {
        accumCountNew = 0.0;
    }

    if (FrameDataCB.IsDenoiserEnabled)
    {
        GatheredRGBA shadowedShadingDenoisedGatherResult = GatherRGBAManually(shadowedShadingDenoisedHistoryTexture, bilinearFilterAtPrevPos, 0.0, PointClampSampler());
        GatheredRGBA unshadowedShadingDenoisedGatherResult = GatherRGBAManually(unshadowedShadingDenoisedHistoryTexture, bilinearFilterAtPrevPos, 0.0, PointClampSampler());

        float4 shadowedShadingDenoisedReprojected = float4(
            ApplyBilinearCustomWeights(shadowedShadingDenoisedGatherResult.Red, weights),
            ApplyBilinearCustomWeights(shadowedShadingDenoisedGatherResult.Green, weights),
            ApplyBilinearCustomWeights(shadowedShadingDenoisedGatherResult.Blue, weights),
            ApplyBilinearCustomWeights(shadowedShadingDenoisedGatherResult.Alpha, weights));

        shadowedShadingReprojectionTarget[pixelIndex] = shadowedShadingDenoisedReprojected;

        float3 unshadowedShadingDenoisedReprojected = float3(
            ApplyBilinearCustomWeights(unshadowedShadingDenoisedGatherResult.Red, weights),
            ApplyBilinearCustomWeights(unshadowedShadingDenoisedGatherResult.Green, weights),
            ApplyBilinearCustomWeights(unshadowedShadingDenoisedGatherResult.Blue, weights));

        unshadowedShadingReprojectionTarget[pixelIndex].rgb = unshadowedShadingDenoisedReprojected;

        float2 reprojectedTexelIdx = occlusion.x > 0.0 ? UVToTexelIndex(reprojectedUV, GlobalDataCB.PipelineRTResolution) : -1.0;
        reprojectedTexelIndicesTarget[pixelIndex].rg = reprojectedTexelIdx;
    }

    currentAccumulationCounterTexture[pixelIndex] = accumCountNew;
}

#endif