#ifndef _DenoiserForwardProjection__
#define _DenoiserForwardProjection__

#include "ColorConversion.hlsl"
#include "Random.hlsl"
#include "Utils.hlsl"
#include "Matrix.hlsl" 
#include "DenoiserCommon.hlsl"
#include "ThreadGroupTilingX.hlsl"

struct PassData
{
    uint2 DispatchGroupCount;
    uint ViewDepthPrevTexIdx;
    uint AlbedoMetalnessPrevTexIdx;
    uint NormalRoughnessPrevTexIdx;
    uint ReprojectedTexelIndicesTexIdx;
    uint StochasticShadowedShadingPrevTexIdx;
    uint StochasticRngSeedsPrevTexIdx;
    uint GradientSamplePositionsPrevTexIdx;
    uint StochasticRngSeedsOutputTexIdx;
    uint GradientSamplePositionsOutputTexIdx;
    uint GradientSamplesOutputTexIdx;
    uint AlbedoMetalnessOutputTexIdx;
    uint NormalRoughnessOutputTexIdx;
    uint ViewDepthOutputTexIdx;
    uint FrameNumber;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const uint GroupDimensionSize = 8;

// https://github.com/NVIDIA/Q2RTX/blob/master/src/refresh/vkpt/shader/asvgf_gradient_reproject.comp

void Reproject(uint2 pixelIdx, inout float luminance, inout uint2 reprojectedTexelIdx, inout bool validSample)
{
    float2 uv = TexelIndexToUV(pixelIdx, GlobalDataCB.PipelineRTResolution);

    // ========================================================================================================================== //
    Texture2D reprojectedTexelIndicesTexture = Textures2D[PassDataCB.ReprojectedTexelIndicesTexIdx];
    Texture2D shadowedShadingPrevTexture = Textures2D[PassDataCB.StochasticShadowedShadingPrevTexIdx];
    Texture2D<uint4> gradientSamplePositionsPrevTexture = UInt4_Textures2D[PassDataCB.GradientSamplePositionsPrevTexIdx];
    // ========================================================================================================================== //

    validSample = true;
    float2 reprojectedTexelIdxFloat = reprojectedTexelIndicesTexture[pixelIdx].xy;
    reprojectedTexelIdx = reprojectedTexelIdxFloat;

    bool occluded = any(reprojectedTexelIdxFloat < 0.0);

    if (occluded)
    {
        validSample = false;
    }

    uint2 reprojectedDownscaledTexelIdx = reprojectedTexelIdx * GradientUpscaleCoefficientInv;
    uint packedStrataPosition = gradientSamplePositionsPrevTexture[reprojectedDownscaledTexelIdx].x;
    uint2 prevStrataPos = packedStrataPosition == InvalidStrataPositionIndicator ? 0 : UnpackStratumPosition(packedStrataPosition);

    // If this pixel was a gradient on the previous frame, don't use it. 
    // Carrying forward the same random number sequence over multiple frames introduces bias.
    bool wasUsedAsGradientSampleInPrevFrame = all((reprojectedDownscaledTexelIdx * GradientUpscaleCoefficient + prevStrataPos) == reprojectedTexelIdx);
    if (wasUsedAsGradientSampleInPrevFrame)
    {
        validSample = false;
    }

    float3 shadowedLuminance = shadowedShadingPrevTexture[reprojectedTexelIdx].rgb;
    luminance = CIELuminance(shadowedLuminance);
}

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
    uint2 downscaledPixelIdx = ThreadGroupTilingX(PassDataCB.DispatchGroupCount, GroupDimensionSize.xx, 8, groupThreadID.xy, groupID.xy);

    // ========================================================================================================================== //
    Texture2D<uint4> rngSeedsPrevTexture = UInt4_Textures2D[PassDataCB.StochasticRngSeedsPrevTexIdx];
    Texture2D previousViewDepthTexture = Textures2D[PassDataCB.ViewDepthPrevTexIdx];
    Texture2D previousAlbedoMetalnessTexture = Textures2D[PassDataCB.AlbedoMetalnessPrevTexIdx];
    Texture2D previousNormalRoughnessTexture = Textures2D[PassDataCB.NormalRoughnessPrevTexIdx];

    RWTexture2D<uint4> rngSeedsOutputTexture = RW_UInt4_Textures2D[PassDataCB.StochasticRngSeedsOutputTexIdx];
    RWTexture2D<uint> gradientSamplePositionsOutputTexture = RW_UInt_Textures2D[PassDataCB.GradientSamplePositionsOutputTexIdx];
    RWTexture2D<float4> gradientOutputTexture = RW_Float4_Textures2D[PassDataCB.GradientSamplesOutputTexIdx];
    RWTexture2D<float4> albedoMetalnessOutputTexture = RW_Float4_Textures2D[PassDataCB.AlbedoMetalnessOutputTexIdx];
    RWTexture2D<float4> normalRoughnessOutputTexture = RW_Float4_Textures2D[PassDataCB.NormalRoughnessOutputTexIdx];
    RWTexture2D<float4> viewDepthOutputTexture = RW_Float4_Textures2D[PassDataCB.ViewDepthOutputTexIdx];
    // ========================================================================================================================== //

    // Find the brightest pixel in the stratum, but _not_ the same one as we used on the previous frame.
    // Picking the brightest pixel helps prevent bright trails when the light has moved.
    // If we just pick a random pixel in the the penumbra of the sun light for example,
    // there is a high chance that this pixel will not receive any sun light due to random sampling of the sun. 
    // Overall, we'll miss the changing luminance of the moving penumbra, which is very well visible.

    // Pull the prev. frame sample position to make sure we're not using the same pixel. 
    // It's important because keeping the same random number sequence for more than one frame
    // introduces a visible bias.

    uint2 maxLuminanceStrataPosition = 0; 
    uint2 maxLuminanceReprojectedTexelIdx = 0;
    float maxLuminance = 0;
    uint2 maxLuminanceStrataPositionFallback = 0;
    uint2 maxLuminanceReprojectedTexelIdxFallback = 0;
    float maxLuminanceFallback = 0;
    bool found = false;
    bool noValidSamples = true;

    SetDataInspectorWriteCondition(all(FrameDataCB.MousePosition / 3 == downscaledPixelIdx));

    // Go over values from the previous frame to find the brightest pixel.
    [unroll]
    for (int y = 0; y < GradientUpscaleCoefficient; y++)
    {
        [unroll]
        for (int x = 0; x < GradientUpscaleCoefficient; x++)
        {
            uint2 strataPosition = uint2(x, y);
            uint2 pixelIdx = downscaledPixelIdx * GradientUpscaleCoefficient + strataPosition;

            float luminance;
            uint2 reprojectedTexelIdx;
            bool validSample;
            Reproject(pixelIdx, luminance, reprojectedTexelIdx, validSample);

            if (luminance > maxLuminance && validSample)
            {
                // We have a good non-zero sample that is not occluded 
                // and wasn't a gradient sample in previous frame
                maxLuminance = luminance;
                maxLuminanceStrataPosition = strataPosition;
                maxLuminanceReprojectedTexelIdx = reprojectedTexelIdx;
                found = true;
                noValidSamples = false;
            }
            else if (validSample)
            {
                // Black sample, but still valid, will serve as a fallback in a case when all valid samples are black
                maxLuminanceFallback = luminance;
                maxLuminanceStrataPositionFallback = strataPosition;
                maxLuminanceReprojectedTexelIdxFallback = reprojectedTexelIdx;
                noValidSamples = false;
            }
        }
    }

    if (!found)
    {
        maxLuminance = maxLuminanceFallback;
        maxLuminanceStrataPosition = maxLuminanceStrataPositionFallback;
        maxLuminanceReprojectedTexelIdx = maxLuminanceReprojectedTexelIdxFallback;
    }

    if (noValidSamples)
    {
        gradientSamplePositionsOutputTexture[downscaledPixelIdx].r = InvalidStrataPositionIndicator;
        return;
    }

    // Index of pixel that we want to patch with data from previous frame
    uint2 patchPixelIdx = downscaledPixelIdx * GradientUpscaleCoefficient + maxLuminanceStrataPosition; 

    // Reuse rng seeds from previous frame for gradient pixels
    rngSeedsOutputTexture[patchPixelIdx] = float4(rngSeedsPrevTexture[maxLuminanceReprojectedTexelIdx].xyz, true);

    // Reuse depth values from previous frame so that subsequent render passes could reconstruct exact world position from previous frame
    // Depth buffer patching is important to avoid false positives when camera movement and/or TAA jitter is involved
    viewDepthOutputTexture[patchPixelIdx] = previousViewDepthTexture[maxLuminanceReprojectedTexelIdx];

    // Same for other gbuffer textures
    normalRoughnessOutputTexture[patchPixelIdx] = previousNormalRoughnessTexture[maxLuminanceReprojectedTexelIdx];
    albedoMetalnessOutputTexture[patchPixelIdx] = previousAlbedoMetalnessTexture[maxLuminanceReprojectedTexelIdx];

    // Output gradient sample
    gradientOutputTexture[downscaledPixelIdx].r = maxLuminance;

    // Save local index of the sample so that we could avoid using the same pixel as sample again in the next frame
    gradientSamplePositionsOutputTexture[downscaledPixelIdx].r = PackStratumPosition(maxLuminanceStrataPosition);
}

#endif