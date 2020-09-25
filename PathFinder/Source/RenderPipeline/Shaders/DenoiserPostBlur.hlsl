#ifndef _DenoiserPostBlur__
#define _DenoiserPostBlur__

#include "ThreadGroupTilingX.hlsl"
#include "Random.hlsl"
#include "Constants.hlsl"
#include "Utils.hlsl"
#include "DenoiserCommon.hlsl"
#include "ColorConversion.hlsl"

struct PassData
{
    uint2 DispatchGroupCount;
    uint AccumulatedFramesCountTexIdx;
    uint AnalyticShadingTexIdx;
    uint SecondaryGradientTexIdx;
    uint ShadowedShadingTexIdx;
    uint UnshadowedShadingTexIdx;
    uint ShadowedShadingBlurredOutputTexIdx;
    uint UnshadowedShadingBlurredOutputTexIdx;
    uint CombinedShadingTexIdx;
    uint CombinedShadingOversaturatedTexIdx;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const int GroupDimensionSize = 16;
static const int BlurSampleCount = 8;

float3 Blur(Texture2D image, float2 uv, uint2 pixelIdx, float2 texelSize, float gradient, float diskRotation)
{
    float3 blurred = image[pixelIdx].rgb;

    if (FrameDataCB.IsDenoiserEnabled)
    {
        return blurred;
    }

    float2 diskScale = texelSize * BlurSampleCount * gradient;
    int sampleCount = BlurSampleCount * gradient;

    for (int i = 0; i < sampleCount; ++i)
    {
        // Generate sample in 2D
        float2 vdSample = VogelDiskSample(i, BlurSampleCount, diskRotation);
        float2 sampleUV = uv + vdSample * diskScale;

        // Sample neighbor value and weight accordingly
        blurred += image.SampleLevel(LinearClampSampler(), sampleUV, 0).rgb;
    }

    return blurred / (sampleCount + 1);
}

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(uint3 groupThreadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
    uint2 pixelIndex = ThreadGroupTilingX(PassDataCB.DispatchGroupCount, GroupDimensionSize.xx, 16, groupThreadID.xy, groupID.xy);
    float2 uv = TexelIndexToUV(pixelIndex, GlobalDataCB.PipelineRTResolution);
    float2 texelSize = 1.0 * GlobalDataCB.PipelineRTResolutionInv;

    Texture2D accumulatedFramesCountTexture = Textures2D[PassDataCB.AccumulatedFramesCountTexIdx];
    Texture2D gradientTexture = Textures2D[PassDataCB.SecondaryGradientTexIdx];
    Texture2D analyticShadingTexture = Textures2D[PassDataCB.AnalyticShadingTexIdx];
    Texture2D shadowedShadingTexture = Textures2D[PassDataCB.ShadowedShadingTexIdx];
    Texture2D unshadowedShadingTexture = Textures2D[PassDataCB.UnshadowedShadingTexIdx];

    RWTexture2D<float4> shadowedShadingBlurredOutputTexture = RW_Float4_Textures2D[PassDataCB.ShadowedShadingBlurredOutputTexIdx];
    RWTexture2D<float4> unshadowedShadingBlurredOutputTexture = RW_Float4_Textures2D[PassDataCB.UnshadowedShadingBlurredOutputTexIdx];
    RWTexture2D<float4> combinedShadingTargetTexture = RW_Float4_Textures2D[PassDataCB.CombinedShadingTexIdx];
    RWTexture2D<float4> combinedOversaturatedShadingTargetTexture = RW_Float4_Textures2D[PassDataCB.CombinedShadingOversaturatedTexIdx];

    // Get a random rotation to be applied for each sample
    float vogelDiskRotation = Random(pixelIndex.xy) * TwoPi;
    float2 gradients = gradientTexture[pixelIndex].rg;

    float3 stochasticShadowed = Blur(shadowedShadingTexture, uv, pixelIndex, texelSize, gradients.x, vogelDiskRotation);
    float3 stochasticUnshadowed = Blur(unshadowedShadingTexture, uv, pixelIndex, texelSize, gradients.x, vogelDiskRotation);

    float3 combinedShading = CombineShading(analyticShadingTexture[pixelIndex].rgb, stochasticShadowed, stochasticUnshadowed);

    if (FrameDataCB.IsReprojectionHistoryDebugEnabled ||
        FrameDataCB.IsGradientDebugEnabled ||
        FrameDataCB.IsMotionDebugEnabled)
    {
        // Debug data is encoded in gradients texture
        if (any(gradients < 0.7))
        {
            combinedShading = float3(1.0 - gradients, 0.0);
        }
    }

    combinedShadingTargetTexture[pixelIndex].rgb = combinedShading;
    combinedOversaturatedShadingTargetTexture[pixelIndex].rgb = CIELuminance(combinedShading) > 1.0 ? combinedShading : 0.0;
}

#endif