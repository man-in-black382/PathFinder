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
    uint ShadowedShadingMip0TexIdx;
    uint UnshadowedShadingMip0TexIdx;
    uint ShadowedShadingTailMipsTexIdx;
    uint UnshadowedShadingTailMipsTexIdx;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const int GroupDimensionSize = 16;
static const int HistoryFixMipCount = MaxFrameCountWithHistoryFix + 1;
static const float BilateralFilterViewZSensitivity = 0.05;

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 DTid : SV_DispatchThreadID, int3 GTid : SV_GroupThreadID)
{
    // Some thoughts on depth-aware bilateral upsampling
    // http://c0de517e.blogspot.com/2016/02/downsampled-effects-with-depth-aware.html

    uint2 pixelIndex = DTid.xy;
    float2 uv = (float2(pixelIndex) + 0.5) * GlobalDataCB.PipelineRTResolutionInv; 

    Texture2D normalRoughnessTexture = Textures2D[PassDataCB.GBufferNormalRoughnessTexIdx];
    Texture2D viewDepthTexture = Textures2D[PassDataCB.ViewDepthTexIdx];
    Texture2D accumulationCounterTexture = Textures2D[PassDataCB.AccumulationCounterTexIdx];
    Texture2D shadowedShadingTailMips = Textures2D[PassDataCB.ShadowedShadingTailMipsTexIdx];
    Texture2D unshadowedShadingTailMips = Textures2D[PassDataCB.UnshadowedShadingTailMipsTexIdx];

    RWTexture2D<float4> shadowedShadingMip0 = RW_Float4_Textures2D[PassDataCB.ShadowedShadingMip0TexIdx];
    RWTexture2D<float4> unshadowedShadingMip0 = RW_Float4_Textures2D[PassDataCB.UnshadowedShadingMip0TexIdx];

    float accumulatedFramesCount = accumulationCounterTexture[pixelIndex].r;
    float normAccumulatedFrameCount = accumulatedFramesCount * MaxFrameCountWithHistoryFixInv;

    // Enough frames are accumulated to stop history reconstruction
    if (normAccumulatedFrameCount >= 1.0)
    {
        return;
    }

    float roughness;
    float3 surfaceNormal;
    LoadGBufferNormalAndRoughness(normalRoughnessTexture, pixelIndex, surfaceNormal, roughness);

    uint mipLevel = (HistoryFixMipCount - 1)* (1.0 - normAccumulatedFrameCount)* roughness;

    // Sampling mip 0 and then writing to mip 0 is useless work
    if (mipLevel == 0)
    {
        return;
    }

    float2 mipSize = GlobalDataCB.PipelineRTResolution / (1 << mipLevel);
    Bilinear bilinearFilter = GetBilinearFilter(uv, 1.0 / mipSize, mipSize);

    float baseViewDepth = viewDepthTexture.SampleLevel(PointClampSampler, uv, 0.0).r;
    float4 mipViewDepths = GatherRedManually(viewDepthTexture, bilinearFilter, mipLevel, PointClampSampler);

    float4 depthsRelativeDistance = abs(baseViewDepth / mipViewDepths - 1.0);
    float4 weights = GetBilinearCustomWeights(bilinearFilter, 1.0);

    // Nearest-Depth Upsampling
    // http://developer.download.nvidia.com/assets/gamedev/files/sdk/11/OpacityMappingSDKWhitePaper.pdf
    //
    float4 depthDifferences = abs(baseViewDepth - mipViewDepths);

    if (any(depthsRelativeDistance > BilateralFilterViewZSensitivity))
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

    GatheredRGB shadowedShadingGatherResult = GatherRGBManually(shadowedShadingTailMips, bilinearFilter, mipLevel, PointClampSampler);
    GatheredRGB unshadowedShadingGatherResult = GatherRGBManually(unshadowedShadingTailMips, bilinearFilter, mipLevel, PointClampSampler);

    float3 shadowedShadingFixed = float3(
        ApplyBilinearCustomWeights(shadowedShadingGatherResult.Red, weights),
        ApplyBilinearCustomWeights(shadowedShadingGatherResult.Green, weights),
        ApplyBilinearCustomWeights(shadowedShadingGatherResult.Blue, weights));

    float3 unshadowedShadingFixed = float3(
        ApplyBilinearCustomWeights(unshadowedShadingGatherResult.Red, weights),
        ApplyBilinearCustomWeights(unshadowedShadingGatherResult.Green, weights),
        ApplyBilinearCustomWeights(unshadowedShadingGatherResult.Blue, weights));

    shadowedShadingMip0[pixelIndex].rgb = shadowedShadingFixed;
    unshadowedShadingMip0[pixelIndex].rgb = unshadowedShadingFixed;
}

#endif