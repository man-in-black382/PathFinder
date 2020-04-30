#ifndef _ShadowNoiseEstimationDenoising__
#define _ShadowNoiseEstimationDenoising__

#include "Utils.hlsl"

struct PassData
{
    uint NoiseEstimationTextureIndex;
    uint OutputTextureIndex;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

//// Number of lines to measure total variance with
//static const int VarianceMeasurementLinesCount = 3;
//static const int EstimationRadius = 3;
//static const float EstimationRadiusInverse = 1.0 / float(EstimationRadius);
static const int GroupDimensionSize = 16;
//static const int GSBufferDimensionSize = EstimationRadius * 2 + GroupDimensionSize; // Add EstimationRadius pixels around the group

// 32KB Max
//groupshared float gLuminances;

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    Texture2D noiseEstimation = Textures2D[PassDataCB.NoiseEstimationTextureIndex];
    RWTexture2D<float4> output = RW_Float4_Textures2D[PassDataCB.OutputTextureIndex];

    static const int R = 1;
    float denoisedEstimation = 0.0;

    [unroll]
    for (int x = -R; x <= R; ++x)
    {
        [unroll]
        for (int y = -R; y <= R; ++y)
        {
            int2 loadIndex = clamp(dispatchThreadID.xy + int2(x, y), 0, GlobalDataCB.PipelineRTResolution - 1);
            denoisedEstimation += noiseEstimation[loadIndex].r;
        }
    }

    denoisedEstimation *= (1.0 / Square(2.0 * float(R) + 1.0));

    output[dispatchThreadID.xy] = denoisedEstimation;
}

#endif