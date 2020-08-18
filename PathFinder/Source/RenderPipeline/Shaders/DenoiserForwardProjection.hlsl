#ifndef _DenoiserForwardProjection__
#define _DenoiserForwardProjection__

#include "ShadingPacking.hlsl"

struct PassData
{
    uint GBufferViewDepthPrevTexIdx;
    uint StochasticShadowedShadingPrevTexIdx;
    uint StochasticUnshadowedShadingPrevTexIdx;
    uint StochasticRngSeedsPrevTexIdx;
    uint GradientSamplePositionsPrevTexIdx;
    uint StochasticRngSeedsTexIdx;
    uint GradientSamplePositionsTexIdx;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const int GroupDimensionSize = 16;

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    uint2 pixelIndex = dispatchThreadID.xy;
    float2 uv = (float2(pixelIndex) + 0.5) / (GlobalDataCB.PipelineRTResolution);

   /* Texture2D shadowedShadingTexture = Textures2D[PassDataCB.ShadingShadowedTexIdx];
    Texture2D unshadowedShadingTexture = Textures2D[PassDataCB.ShadingUnshadowedTexIdx];
    Texture2D shadowedShadingHistoryTexture = Textures2D[PassDataCB.ShadingShadowedHistoryTexIdx];
    Texture2D unshadowedShadingHistoryTexture = Textures2D[PassDataCB.ShadingUnshadowedHistoryTexIdx];

    RWTexture2D<float4> shadowedShadingGradientTexture = RW_Float4_Textures2D[PassDataCB.ShadingShadowedGradientTexIdx];
    RWTexture2D<float4> unshadowedShadingGradientTexture = RW_Float4_Textures2D[PassDataCB.ShadingUnshadowedGradientTexIdx];*/
}

#endif