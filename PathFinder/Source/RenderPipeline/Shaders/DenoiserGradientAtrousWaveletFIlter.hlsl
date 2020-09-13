#ifndef _DenoiserGradientAtrousWaveletFilter__
#define _DenoiserGradientAtrousWaveletFilter__

struct PassCBData
{
    uint2 ImageSize;
    uint InputTexIdx;
    uint OutputTexIdx;
    uint CurrentIteration;
};

#define PassDataType PassCBData

#include "MandatoryEntryPointInclude.hlsl"

static const float WaveletFactor = 0.5;
static const float WaveletKernel[2][2] = {
    { 1.0, WaveletFactor  },
    { WaveletFactor, WaveletFactor * WaveletFactor }
};

static const int GroupDimensionSize = 16;

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID)
{
    uint2 pixelIdx = dispatchThreadID.xy;

    RWTexture2D<float4> inputTexture = RW_Float4_Textures2D[PassDataCB.InputTexIdx];
    RWTexture2D<float4> outputTexture = RW_Float4_Textures2D[PassDataCB.OutputTexIdx];

    float2 gradients = inputTexture[pixelIdx].rg; 
    float totalWeight = 1.0;
    float2 sum = 0.0;

    const int StepSize = int(1u << PassDataCB.CurrentIteration);

    const int r = 1; 
    for (int yy = -r; yy <= r; yy++)
    {
        for (int xx = -r; xx <= r; xx++)
        {
            int2 p = pixelIdx + int2(xx, yy) * StepSize;
            float2 c = inputTexture[p].rg;

            if (any(p >= PassDataCB.ImageSize))
            {
                c = 0.0;
            }

            float weight = WaveletKernel[abs(xx)][abs(yy)];

            sum += c * weight;
            totalWeight += weight;
        }
    }

    sum /= totalWeight;

    outputTexture[pixelIdx].rg = sum;
}

#endif