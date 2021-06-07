#ifndef _DenoiserForwardProjection__
#define _DenoiserForwardProjection__

#include "ColorConversion.hlsl"
#include "Random.hlsl"
#include "Utils.hlsl"
#include "DenoiserCommon.hlsl"
#include "ThreadGroupTilingX.hlsl"

struct PassData
{
    uint2 DispatchGroupCount;
    uint DepthStencilTexIdx;
    uint ReprojectedTexelIndicesTexIdx;
    uint StochasticShadowedShadingPrevTexIdx;
    uint StochasticRngSeedsPrevTexIdx;
    uint GradientSamplePositionsPrevTexIdx;
    uint StochasticRngSeedsOutputTexIdx;
    uint GradientSamplePositionsOutputTexIdx;
    uint GradientSamplesOutputTexIdx;
    uint FrameNumber;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const uint GroupDimensionSize = 8;

// https://github.com/NVIDIA/Q2RTX/blob/master/src/refresh/vkpt/shader/asvgf_gradient_reproject.comp

void Reproject(uint2 pixelIdx, inout float luminance, inout uint2 reprojectedTexelIdx)
{
    float2 uv = TexelIndexToUV(pixelIdx, GlobalDataCB.PipelineRTResolution);

    // ========================================================================================================================== //
    Texture2D reprojectedTexelIndicesTexture = Textures2D[PassDataCB.ReprojectedTexelIndicesTexIdx];
    Texture2D shadowedShadingPrevTexture = Textures2D[PassDataCB.StochasticShadowedShadingPrevTexIdx];
    Texture2D<uint4> gradientSamplePositionsPrevTexture = UInt4_Textures2D[PassDataCB.GradientSamplePositionsPrevTexIdx];
    // ========================================================================================================================== //

    reprojectedTexelIdx = reprojectedTexelIndicesTexture[pixelIdx].xy;

    bool occluded = any(reprojectedTexelIdx < 0.0);

    if (occluded)
    {
        luminance = 0.0; 
        return;
    }

    uint2 reprojectedDownscaledTexelIdx = reprojectedTexelIdx * GradientUpscaleCoefficientInv;
    uint2 prevStrataPos = UnpackStratumPosition(gradientSamplePositionsPrevTexture[reprojectedDownscaledTexelIdx].x);

    // If this pixel was a gradient on the previous frame, don't use it. 
    // Carrying forward the same random number sequence over multiple frames introduces bias.
    bool wasUsedAsGradientSampleInPrevFrame = all((reprojectedDownscaledTexelIdx * GradientUpscaleCoefficient + prevStrataPos) == reprojectedTexelIdx);
    if (wasUsedAsGradientSampleInPrevFrame)
    {
        luminance = 0.0;
        return;
    }

    float3 shadowedLuminance = shadowedShadingPrevTexture[reprojectedTexelIdx].rgb;
    luminance = CIELuminance(shadowedLuminance);
}

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(uint3 groupThreadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
    uint2 downscaledPixelIdx = ThreadGroupTilingX(PassDataCB.DispatchGroupCount, GroupDimensionSize.xx, 8, groupThreadID.xy, groupID.xy);

    // ========================================================================================================================== //
    Texture2D<uint4> rngSeedsPrevTexture = UInt4_Textures2D[PassDataCB.StochasticRngSeedsPrevTexIdx];
    Texture2D depthStencilTexture = Textures2D[PassDataCB.DepthStencilTexIdx];

    RWTexture2D<uint4> rngSeedsOutputTexture = RW_UInt4_Textures2D[PassDataCB.StochasticRngSeedsOutputTexIdx];
    RWTexture2D<uint> gradientSamplePositionsOutputTexture = RW_UInt_Textures2D[PassDataCB.GradientSamplePositionsOutputTexIdx];
    RWTexture2D<float4> gradientOutputTexture = RW_Float4_Textures2D[PassDataCB.GradientSamplesOutputTexIdx];
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
    bool found = false;

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
            Reproject(pixelIdx, luminance, reprojectedTexelIdx);

            if (luminance > maxLuminance)
            {
                maxLuminance = luminance;
                maxLuminanceStrataPosition = strataPosition;
                maxLuminanceReprojectedTexelIdx = reprojectedTexelIdx;
                found = true;
            }
        }
    }
   
    if (!found)
    {
        gradientSamplePositionsOutputTexture[downscaledPixelIdx].r = 0;
        return;
    }

    // Index of pixel that we want to patch with data from previous frame
    uint2 patchPixelIdx = downscaledPixelIdx * GradientUpscaleCoefficient + maxLuminanceStrataPosition;

    // Reuse rng seeds from previous frame for gradient pixels
    rngSeedsOutputTexture[patchPixelIdx] = float4(rngSeedsPrevTexture[maxLuminanceReprojectedTexelIdx].xyz, true);

    // Output gradient sample
    gradientOutputTexture[downscaledPixelIdx].r = maxLuminance;

    // Save local index of the sample so that we could avoid using the same pixel as sample again in the next frame
    gradientSamplePositionsOutputTexture[downscaledPixelIdx].r = PackStratumPosition(maxLuminanceStrataPosition);
}

#endif