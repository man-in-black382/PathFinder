#ifndef _ToneMappingRenderPass__
#define _ToneMappingRenderPass__

#include "GTTonemapping.hlsl"
#include "Exposure.hlsl"
#include "GBuffer.hlsl"

struct PassData
{
    uint InputTexIdx;
    uint OutputTexIdx;
    bool IsHDREnabled;
    float DisplayMaxLuminance;
    // 16 byte boundary
    GTTonemappingParams TonemappingParams;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"
#include "ColorConversion.hlsl"

RWStructuredBuffer<uint> Histogram : register(u0);

static const int HistogramBinCount = 128;

uint GetHistogramBin(float luminance, float minLuminance, float maxLuminance)
{
    float range = maxLuminance - minLuminance;
    if (range < 0.0001)
        return 0;

    return ((luminance - minLuminance) / range) * (HistogramBinCount - 1);
}

groupshared uint gHistogram[HistogramBinCount];

[numthreads(16, 16, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int groupIndex : SV_GroupIndex)
{
    Texture2D inputImage = Textures2D[PassDataCB.InputTexIdx];
    RWTexture2D<float4> outputImage = RW_Float4_Textures2D[PassDataCB.OutputTexIdx];

    float3 color = inputImage.mips[0][dispatchThreadID.xy].rgb;
    color = ExposeLuminance(color, FrameDataCB.CurrentFrameCamera);

    GTTonemappingParams params = PassDataCB.TonemappingParams;
    // Luminance was exposed using Saturation Based Sensitivity method 
    // hence the 1.0 for maximum luminance
    params.MaximumLuminance = 1.0;

    // NaN detection for convenience
    if (any(isnan(color)))
    {
        color = float3(10, 0, 0);
        outputImage[dispatchThreadID.xy] = float4(color, 1.0);
        return;
    }

    color = float3(
        GTToneMap(color.r, params),
        GTToneMap(color.g, params),
        GTToneMap(color.b, params));

    // Histogram computation
    if (groupIndex < HistogramBinCount)
    {
        gHistogram[groupIndex] = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    float2 minMaxLum = float2(0.0, 1.0);
    uint bin = GetHistogramBin(CIELuminance(color), minMaxLum.x, minMaxLum.y);
    uint prevValue;
    InterlockedAdd(gHistogram[bin], 1, prevValue);
    GroupMemoryBarrierWithGroupSync();

    if (groupIndex < HistogramBinCount)
    {
        InterlockedAdd(Histogram[groupIndex], gHistogram[groupIndex], prevValue);
    }

    // Color gamut and quantizer conversions
    if (PassDataCB.IsHDREnabled)
    {
        // Remap tone mapped 1.0 value to correspond to maximum luminance of the display
        const float ST2084Max = 10000.0;
        const float HDRScalar = PassDataCB.DisplayMaxLuminance / ST2084Max;

        // The HDR scene is in Rec.709, but the display is Rec.2020
        color = Rec709ToRec2020(color);

        // Apply the ST.2084 curve to the scene.
        color = LinearToST2084(color * HDRScalar);
    }
    else
    {
        color = LinearToSRGB(color);
    }

    outputImage[dispatchThreadID.xy] = float4(color, 1.0);
}

#endif