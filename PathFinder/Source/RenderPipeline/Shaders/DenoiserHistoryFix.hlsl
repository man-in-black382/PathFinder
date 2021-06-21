#ifndef _DenoiserHistoryFix__
#define _DenoiserHistoryFix__

#include "DenoiserCommon.hlsl"
#include "GBuffer.hlsl"
#include "Utils.hlsl"
#include "Filtering.hlsl"

struct PassData
{
    uint GBufferNormalRoughnessTexIdx;
    uint ViewDepthTexIdx;
    uint AccumulationCounterTexIdx;
    uint ShadowedShadingFixedTexIdx;
    uint UnshadowedShadingFixedTexIdx;
    uint ShadowedShadingPreBlurredTexIdx;
    uint UnshadowedShadingPreBlurredTexIdx;
    bool IsDenoiserEnabled;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const int GroupDimensionSize = 8;
static const int HistoryFixMipCount = MaxFrameCountWithHistoryFix + 1;
static const float BilateralFilterViewZSensitivity = 0.1;

void Fix(Texture2D inputTexture, RWTexture2D<float4> outputTexture, Texture2D viewDepthTexture, float roughness, float historyWeight, float baseViewDepth, uint2 pixelIndex, float2 uv)
{
    uint mipLevel = (HistoryFixMipCount - 1) * saturate(1.0 - historyWeight) * roughness;

    // Sampling mip 0 and then writing to mip 0 is useless work
    if (mipLevel == 0)
    {
        outputTexture[pixelIndex] = inputTexture.mips[0][pixelIndex];
        return;
    }

    float2 mipSize = GlobalDataCB.PipelineRTResolution / (1 << mipLevel);
    Bilinear bilinearFilter = GetBilinearFilter(uv, 1.0 / mipSize, mipSize);

    float4 mipViewDepths = GatherRedManually(viewDepthTexture, bilinearFilter, mipLevel, PointClampSampler());
    float4 weights = GetBilinearCustomWeights(bilinearFilter, 1.0);

    // Some thoughts on depth-aware bilateral upsampling
    // http://c0de517e.blogspot.com/2016/02/downsampled-effects-with-depth-aware.html

    // Nearest-Depth Upsampling
    // http://developer.download.nvidia.com/assets/gamedev/files/sdk/11/OpacityMappingSDKWhitePaper.pdf
    //
    float4 depthDifferences = abs(baseViewDepth - mipViewDepths);

    // If any close enough depth exists, pick it,
    // otherwise accept noise and don't do the upsampling,
    // because the produced ghosting is visually worse than the noise itself
    if (any(depthDifferences < BilateralFilterViewZSensitivity))
    {
        int closestSampleIdx = 0;
        float diff = depthDifferences[0];

        [unroll]
        for (int i = 1; i < 4; ++i)
        {
            if (depthDifferences[i] < diff)
            {
                diff = depthDifferences[i];
                closestSampleIdx = i;
            }
        }

        weights = 0.0;
        weights[closestSampleIdx] = 1.0;
    }
    else
    {
        outputTexture[pixelIndex] = inputTexture.mips[0][pixelIndex];
        return;
    }

    GatheredRGBA gatherResult = GatherRGBAManually(inputTexture, bilinearFilter, mipLevel, PointClampSampler());

    float4 fixedValues = float4(
        ApplyBilinearCustomWeights(gatherResult.Red, weights),
        ApplyBilinearCustomWeights(gatherResult.Green, weights),
        ApplyBilinearCustomWeights(gatherResult.Blue, weights),
        ApplyBilinearCustomWeights(gatherResult.Alpha, weights));

    outputTexture[pixelIndex] = fixedValues;
}

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 DTid : SV_DispatchThreadID, int3 GTid : SV_GroupThreadID)
{
    if (!FrameDataCB.IsDenoiserEnabled)
    {
        return;
    }

    uint2 pixelIndex = DTid.xy;
    float2 uv = (float2(pixelIndex) + 0.5) * GlobalDataCB.PipelineRTResolutionInv; 

    Texture2D normalRoughnessTexture = Textures2D[PassDataCB.GBufferNormalRoughnessTexIdx];
    Texture2D viewDepthTexture = Textures2D[PassDataCB.ViewDepthTexIdx];
    Texture2D accumulationCounterTexture = Textures2D[PassDataCB.AccumulationCounterTexIdx];
    Texture2D shadowedShadingPreBlurred = Textures2D[PassDataCB.ShadowedShadingPreBlurredTexIdx];
    Texture2D unshadowedShadingPreBlurred = Textures2D[PassDataCB.UnshadowedShadingPreBlurredTexIdx];

    RWTexture2D<float4> shadowedShadingFixedTexture = RW_Float4_Textures2D[PassDataCB.ShadowedShadingFixedTexIdx];
    RWTexture2D<float4> unshadowedShadingFixedTexture = RW_Float4_Textures2D[PassDataCB.UnshadowedShadingFixedTexIdx];

    float accumulatedFramesCount = accumulationCounterTexture[pixelIndex].r;
    float normAccumulatedFrameCount = accumulatedFramesCount * MaxFrameCountWithHistoryFixInv;

    // Separate weights for shadowed and unshadowed stochastic shading textures
    float2 historyWeights = normAccumulatedFrameCount.xx;

    float roughness;
    float3 surfaceNormal;
    LoadGBufferNormalAndRoughness(normalRoughnessTexture, pixelIndex, surfaceNormal, roughness);

    float baseViewDepth = viewDepthTexture[pixelIndex].r;

    Fix(shadowedShadingPreBlurred, shadowedShadingFixedTexture, viewDepthTexture, roughness, historyWeights.x, baseViewDepth, pixelIndex, uv);
    Fix(unshadowedShadingPreBlurred, unshadowedShadingFixedTexture, viewDepthTexture, roughness, historyWeights.y, baseViewDepth, pixelIndex, uv);
}

#endif