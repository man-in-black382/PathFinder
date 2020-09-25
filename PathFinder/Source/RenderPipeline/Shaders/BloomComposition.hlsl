#ifndef _BloomComposition__
#define _BloomComposition__

struct PassData
{
    float2 InverseTextureDimensions;
    uint CombinedShadingTexIdx;
    uint BloomBlurOutputTexIdx;
    uint OutputTexIdx;
    uint SmallBloomWeight;
    uint MediumBloomWeight;
    uint LargeBloomWeight;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"
#include "ColorConversion.hlsl"

[numthreads(32, 32, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D combinedShading = Textures2D[PassDataCB.CombinedShadingTexIdx];
    Texture2D bloomBlurOutput = Textures2D[PassDataCB.BloomBlurOutputTexIdx];
    RWTexture2D<float4> compositionOutput = RW_Float4_Textures2D[PassDataCB.OutputTexIdx];

    float2 centerUV = (float2(dispatchThreadID.xy) + 0.5f) * PassDataCB.InverseTextureDimensions;

    float3 color0 = combinedShading.SampleLevel(LinearClampSampler(), centerUV, 0.0).rgb;
    float3 color1 = bloomBlurOutput.SampleLevel(LinearClampSampler(), centerUV, 0.0).rgb;
    float3 color2 = bloomBlurOutput.SampleLevel(LinearClampSampler(), centerUV, 1.0).rgb;
    float3 color3 = bloomBlurOutput.SampleLevel(LinearClampSampler(), centerUV, 2.0).rgb;

    float totalWeight = PassDataCB.SmallBloomWeight + PassDataCB.MediumBloomWeight + PassDataCB.LargeBloomWeight;
    float3 weights = float3(PassDataCB.SmallBloomWeight, PassDataCB.MediumBloomWeight, PassDataCB.LargeBloomWeight) / totalWeight;
    float3 bloom = color1 * weights.x + color2 * weights.y + color3 * weights.z;
    float bloomScale = 0.05; // Need to figure out a function that'll yield scale based on luminance
    float3 compositedColor = color0 + bloom * bloomScale;

    compositionOutput[dispatchThreadID.xy] = float4(compositedColor, 1.0);
}

#endif