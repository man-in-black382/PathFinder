#ifndef _BloomComposition__
#define _BloomComposition__

struct PassData
{
    float2 InverseTextureDimensions;
    uint DeferredLightingOutputTextureIndex;
    uint BloomBlurOutputTextureIndex;
    uint OutputTextureIndex;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

[numthreads(32, 32, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D deferredLightingOutput = Textures2D[PassDataCB.DeferredLightingOutputTextureIndex];
    Texture2D bloomBlurOutput = Textures2D[PassDataCB.BloomBlurOutputTextureIndex];
    RWTexture2D<float4> compositionOutput = RW_Float4_Textures2D[PassDataCB.OutputTextureIndex];

    float2 centerUV = (float2(dispatchThreadID.xy) + 0.5f) * PassDataCB.InverseTextureDimensions;

    float3 color0 = deferredLightingOutput.SampleLevel(LinearClampSampler, centerUV, 0.0).rgb;
    float3 color1 = bloomBlurOutput.SampleLevel(LinearClampSampler, centerUV, 0.0).rgb;
    float3 color2 = bloomBlurOutput.SampleLevel(LinearClampSampler, centerUV, 1.0).rgb;
    float3 color3 = bloomBlurOutput.SampleLevel(LinearClampSampler, centerUV, 2.0).rgb;

    float3 compositedColor = color0 + color1 + color2 + color3;

    compositionOutput[dispatchThreadID.xy] = float4(compositedColor, 1.0);
}

#endif