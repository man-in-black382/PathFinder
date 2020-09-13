#ifndef _DenoiserGradientConstruction__
#define _DenoiserGradientConstruction__

struct PassCBData
{
    uint GradientInputsTexIdx;
    uint SamplePositionsTexIdx;
    uint ShadowedShadingTexIdx;
    uint UnshadowedShadingTexIdx;
    uint GradientTexIdx;
};

#define PassDataType PassCBData

#include "MandatoryEntryPointInclude.hlsl"
#include "DenoiserCommon.hlsl"
#include "ColorConversion.hlsl"
#include "Constants.hlsl"

static const int GroupDimensionSize = 16;

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    uint2 downscaledPixelIdx = dispatchThreadID.xy;
    uint2 fullPixelIndex = downscaledPixelIdx * GradientUpscaleCoefficient;

    Texture2D gradientInputsTexture = Textures2D[PassDataCB.GradientInputsTexIdx];
    Texture2D shadowedShadingTexture = Textures2D[PassDataCB.ShadowedShadingTexIdx];
    Texture2D unshadowedShadingTexture = Textures2D[PassDataCB.UnshadowedShadingTexIdx];
    Texture2D<uint4> gradientSamplePositionsTexture = UInt4_Textures2D[PassDataCB.SamplePositionsTexIdx];

    RWTexture2D<float4> gradientOutputTexture = RW_Float4_Textures2D[PassDataCB.GradientTexIdx];

    uint2 stratumPosition = UnpackStratumPosition(gradientSamplePositionsTexture[downscaledPixelIdx].x);
    uint2 gradientSamplePixelIdx = fullPixelIndex + stratumPosition;

    float2 lums = float2(
        CIELuminance(shadowedShadingTexture[gradientSamplePixelIdx].rgb),
        CIELuminance(unshadowedShadingTexture[gradientSamplePixelIdx].rgb));

    float2 gradientInputs = gradientInputsTexture[downscaledPixelIdx].rg;

    float2 gradients = float2(GetHFGradient(lums.x, gradientInputs.x), GetHFGradient(lums.y, gradientInputs.y));

    gradientOutputTexture[downscaledPixelIdx].rg = gradients;
}

#endif