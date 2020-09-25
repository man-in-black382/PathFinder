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
    uint GBufferViewDepthPrevTexIdx;
    uint StochasticShadowedShadingPrevTexIdx;
    uint StochasticUnshadowedShadingPrevTexIdx;
    uint StochasticRngSeedsPrevTexIdx;
    uint GradientSamplePositionsPrevTexIdx;
    uint StochasticRngSeedsTexIdx;
    uint GradientSamplePositionsTexIdx;
    uint GradientTexIdx;
    uint FrameNumber;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const int GroupDimensionSize = 16;

// https://github.com/NVIDIA/Q2RTX/blob/master/src/refresh/vkpt/shader/asvgf_fwd_project.comp

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(uint3 groupThreadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
    uint2 prevDownsampledPixelIdx = ThreadGroupTilingX(PassDataCB.DispatchGroupCount, GroupDimensionSize.xx, 16, groupThreadID.xy, groupID.xy);
    uint2 prevPixelIdx = prevDownsampledPixelIdx * GradientUpscaleCoefficient;

    Texture2D GBufferViewDepthPrevTexture = Textures2D[PassDataCB.GBufferViewDepthPrevTexIdx];
    Texture2D shadowedShadingPrevTexture = Textures2D[PassDataCB.StochasticShadowedShadingPrevTexIdx];
    Texture2D unshadowedShadingPrevTexture = Textures2D[PassDataCB.StochasticUnshadowedShadingPrevTexIdx];
    Texture2D<uint4> rngSeedsPrevTexture = UInt4_Textures2D[PassDataCB.StochasticRngSeedsPrevTexIdx];
    Texture2D<uint4> gradientSamplePositionsPrevTexture = UInt4_Textures2D[PassDataCB.GradientSamplePositionsPrevTexIdx];

    RWTexture2D<uint4> rngSeedsOutputTexture = RW_UInt4_Textures2D[PassDataCB.StochasticRngSeedsTexIdx];
    RWTexture2D<uint> gradientSamplePositionsOutputTexture = RW_UInt_Textures2D[PassDataCB.GradientSamplePositionsTexIdx];
    RWTexture2D<float4> gradientOutputTexture = RW_Float4_Textures2D[PassDataCB.GradientTexIdx];

    // Find the brightest pixel in the stratum, but _not_ the same one as we used on the previous frame.
    // Picking the brightest pixel helps prevent bright trails when the light has moved.
    // If we just pick a random pixel in the the penumbra of the sun light for example,
    // there is a high chance that this pixel will not receive any sun light due to random sampling of the sun. 
    // Overall, we'll miss the changing luminance of the moving penumbra, which is very well visible.

    // Pull the prev. frame sample position to make sure we're not using the same pixel. 
    // It's important because keeping the same random number sequence for more than one frame
    // introduces a visible bias.

    uint2 prevStrataPos = UnpackStratumPosition(gradientSamplePositionsPrevTexture[prevDownsampledPixelIdx].x);

    int2 maxLumStrataPos = 0;
    float maxLuminance = 0;
    float2 prevLuminances = 0.0;

    // Go over values from the previous frame to find the brightest pixel.
    for (int yy = 0; yy < GradientUpscaleCoefficient; yy++)
    {
        for (int xx = 0; xx < GradientUpscaleCoefficient; xx++)
        {
            // Same as previous frame - skip
            if (xx == prevStrataPos.x && yy == prevStrataPos.y)
                continue;

            // Pull the colors
            int2 p = prevPixelIdx + int2(xx, yy);
            float shadowedLuminancePrev = CIELuminance(shadowedShadingPrevTexture[p].rgb);
            float unshadowedLuminancePrev = CIELuminance(unshadowedShadingPrevTexture[p].rgb);

            // Use total luminance as the heuristic
            float luminanceSum = shadowedLuminancePrev + unshadowedLuminancePrev;

            if (luminanceSum > maxLuminance)
            {
                maxLuminance = luminanceSum;
                maxLumStrataPos = int2(xx, yy);
                prevLuminances = float2(shadowedLuminancePrev, unshadowedLuminancePrev);
            }
        }
    }

    if (maxLuminance > 0)
    {
        // We found a suitable pixel - use it
        prevPixelIdx += maxLumStrataPos;
    }
    else
    {
        // We didn't find one - all pixels, maybe other than the previously used one, were black.
        // Pick a random pixel in this case.

        uint x = Random(prevDownsampledPixelIdx.x + PassDataCB.FrameNumber) * (GradientUpscaleCoefficient - 1);
        uint y = Random(prevDownsampledPixelIdx.y + PassDataCB.FrameNumber) * (GradientUpscaleCoefficient - 1);

        prevPixelIdx += uint2(x, y);

        prevLuminances = float2(
            CIELuminance(shadowedShadingPrevTexture[prevPixelIdx].rgb),
            CIELuminance(unshadowedShadingPrevTexture[prevPixelIdx].rgb));
    }

    if (any(prevPixelIdx >= GlobalDataCB.PipelineRTResolution))
    {
        return;
    }
       
    // Pixel coordinate of forward projected sample 
    float prevViewDepth = GBufferViewDepthPrevTexture[prevPixelIdx].r;
    float prevDepth = HyperbolizeDepth(prevViewDepth, FrameDataCB.PreviousFrameCamera);

    if (prevViewDepth > FrameDataCB.PreviousFrameCamera.FarPlane)
    {
        return;
    }

    float2 prevUV = TexelIndexToUV(prevPixelIdx, GlobalDataCB.PipelineRTResolution);
    float3 prevWorldPos = NDCDepthToWorldPosition(prevDepth, prevUV, FrameDataCB.PreviousFrameCamera);
    float3 currNDCPos = ViewProject(prevWorldPos, FrameDataCB.CurrentFrameCamera);
    float2 currUV = NDCToUV(currNDCPos);
    uint2 currPixelIdx = UVToTexelIndex(currUV, GlobalDataCB.PipelineRTResolution);

    if (any(currUV) < 0.0 || any(currUV > 1.0))
    {
        return;
    }

    uint2 currDownsampledPixelIdx = currPixelIdx / GradientUpscaleCoefficient;
    uint2 currStratumPos = currPixelIdx % GradientUpscaleCoefficient;

    // Check if this sample is allowed to become a gradient sample
    uint alreadyContainedStratumPos;
    InterlockedCompareExchange(gradientSamplePositionsOutputTexture[currDownsampledPixelIdx], 0u, PackStratumPosition(currStratumPos), alreadyContainedStratumPos);

    if (alreadyContainedStratumPos != 0)
    {
        return;
    }

    //rngSeedsOutputTexture[currPixelIdx] = rngSeedsPrevTexture[prevPixelIdx];
    gradientOutputTexture[currDownsampledPixelIdx].rg = prevLuminances.rg;
}

#endif