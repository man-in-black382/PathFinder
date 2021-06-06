#ifndef _DenoiserPostBlur__
#define _DenoiserPostBlur__

#include "ThreadGroupTilingX.hlsl"
#include "Random.hlsl"
#include "Constants.hlsl"
#include "Utils.hlsl"
#include "DenoiserCommon.hlsl"
#include "ColorConversion.hlsl"
#include "Exposure.hlsl"
#include "GIProbeHelpers.hlsl"
#include "GBuffer.hlsl"

struct PassData
{
    IrradianceField ProbeField;
    GBufferTextureIndices GBufferIndices;
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

float4 Blur(Texture2D image, float2 uv, uint2 pixelIdx, float2 texelSize, float gradient, float diskRotation)
{
    float4 blurred = image[pixelIdx];

    if (!FrameDataCB.IsDenoiserEnabled)
    {
        return blurred;
    }

    float2 diskScale = texelSize * BlurSampleCount * gradient * 0.1;
    int sampleCount = BlurSampleCount * gradient;

    for (int i = 0; i < sampleCount; ++i)
    {
        // Generate sample in 2D
        float2 vdSample = VogelDiskSample(i, BlurSampleCount, diskRotation);
        float2 sampleUV = uv + vdSample * diskScale;

        // Sample neighbor value and weight accordingly
        blurred += image.SampleLevel(LinearClampSampler(), sampleUV, 0);
    }

    return blurred / (sampleCount + 1);
}

float3 RetrieveGI(uint2 texelIndex, float2 uv)
{
    Texture2D irradianceAtlas = Textures2D[PassDataCB.ProbeField.CurrentIrradianceProbeAtlasTexIdx];
    Texture2D depthAtlas = Textures2D[PassDataCB.ProbeField.CurrentDepthProbeAtlasTexIdx];

    GBufferTexturePack gBufferTextures;
    gBufferTextures.AlbedoMetalness = Textures2D[PassDataCB.GBufferIndices.AlbedoMetalnessTexIdx];
    gBufferTextures.NormalRoughness = Textures2D[PassDataCB.GBufferIndices.NormalRoughnessTexIdx];
    gBufferTextures.Motion = UInt4_Textures2D[PassDataCB.GBufferIndices.MotionTexIdx];
    gBufferTextures.TypeAndMaterialIndex = UInt4_Textures2D[PassDataCB.GBufferIndices.TypeAndMaterialTexIdx];
    gBufferTextures.DepthStencil = Textures2D[PassDataCB.GBufferIndices.DepthStencilTexIdx];

    GBufferStandard gBuffer = ZeroGBufferStandard();
    LoadStandardGBuffer(gBuffer, gBufferTextures, texelIndex);

    float depth = gBufferTextures.DepthStencil[texelIndex].r;
    float3 surfacePosition = NDCDepthToWorldPosition(depth, uv, FrameDataCB.CurrentFrameCamera);
    float3 viewDirection = normalize(FrameDataCB.CurrentFrameCamera.Position.xyz - surfacePosition);
    float3 irradiance = RetrieveGIIrradiance(surfacePosition, gBuffer.Normal, viewDirection, irradianceAtlas, depthAtlas, LinearClampSampler(), PassDataCB.ProbeField, false);

    if (FrameDataCB.IsGIIrradianceDebugEnabled)
    {
        return irradiance;
    }
    else
    {
        return DiffuseBRDFForGI(viewDirection, gBuffer) * irradiance;
    }
}

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(uint3 groupThreadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
    uint2 pixelIndex = ThreadGroupTilingX(PassDataCB.DispatchGroupCount, GroupDimensionSize.xx, 16, groupThreadID.xy, groupID.xy);
    float2 uv = TexelIndexToUV(pixelIndex, GlobalDataCB.PipelineRTResolution);
    float2 texelSize = 1.0 * GlobalDataCB.PipelineRTResolutionInv;

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
    float2 gradients = FrameDataCB.IsDenoiserEnabled ? gradientTexture[pixelIndex].rg : 0.0;

    float3 analyticUnshadowed = analyticShadingTexture[pixelIndex].rgb;
    float4 stochasticShadowed = Blur(shadowedShadingTexture, uv, pixelIndex, texelSize, gradients.x, vogelDiskRotation);
    float4 stochasticUnshadowed = Blur(unshadowedShadingTexture, uv, pixelIndex, texelSize, gradients.x, vogelDiskRotation);

    float3 combinedShading = CombineShading(analyticUnshadowed, stochasticShadowed.rgb, stochasticUnshadowed.rgb); 
    float ao = stochasticShadowed.w;

    combinedShading = stochasticShadowed.rgb;
    //combinedShading = stochasticUnshadowed;
    //combinedShading = analyticUnshadowed;

    // Apply GI
 /*   if (FrameDataCB.IsGIEnabled)
    {
        if (FrameDataCB.IsGIIrradianceDebugEnabled)
        {
            combinedShading = RetrieveGI(pixelIndex, uv);
        }
        else
        {
            combinedShading += RetrieveGI(pixelIndex, uv) * ao;
        }
    }*/
    
    //combinedShading = ao;

    if (any(isnan(combinedShading)) || any(isinf(combinedShading)))
        combinedShading = float3(1, 100, 1);

    if (FrameDataCB.IsReprojectionHistoryDebugEnabled ||
        FrameDataCB.IsGradientDebugEnabled ||
        FrameDataCB.IsMotionDebugEnabled)
    {
        // Debug data is encoded in gradients texture
        if (any(gradients > 0.0))
        {
            combinedShading = float3(gradients, 0.0) * 10;
        }
    }

    if (FrameDataCB.IsGradientDebugEnabled)
    {
        if (any(gradients > 0.0))
        {
            combinedShading = float3(gradients, 0.0) * 10;
        }
    }

    float maxLuminance = ConvertEV100ToMaxHsbsLuminance(FrameDataCB.CurrentFrameCamera.ExposureValue100);

    combinedShadingTargetTexture[pixelIndex].rgb = combinedShading;
    combinedOversaturatedShadingTargetTexture[pixelIndex].rgb = CIELuminance(combinedShading) > maxLuminance ? combinedShading : 0.0;
}

#endif