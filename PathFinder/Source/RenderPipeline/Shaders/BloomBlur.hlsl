#ifndef _BloomBlur__
#define _BloomBlur__

#include "GaussianBlur.hlsl"

struct PassCBData
{
    // Packing into 4-component vectors 
    // to satisfy constant buffer alignment rules
    float4 Weights[GaussianBlurMaximumRadius / 4]; 
    float2 ImageSize;
    uint IsHorizontal;
    uint BlurRadius;
    uint InputTexIdx;
    uint OutputTexIdx;
};

#define PassDataType PassCBData

#include "MandatoryEntryPointInclude.hlsl"

[numthreads(GaussianBlurGroupSize, 1, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    RWTexture2D<float4> source = RW_Float4_Textures2D[PassDataCB.InputTexIdx];
    RWTexture2D<float4> destination = RW_Float4_Textures2D[PassDataCB.OutputTexIdx];

    GaussianBlurParameters parameters;
    parameters.Weights = PassDataCB.Weights;
    parameters.ImageSize = PassDataCB.ImageSize;
    parameters.IsHorizontal = PassDataCB.IsHorizontal;
    parameters.BlurRadius = PassDataCB.BlurRadius;

    int2 texelIndex = parameters.IsHorizontal ? dispatchThreadID.xy : dispatchThreadID.yx;
    float3 color = BlurGaussian(texelIndex, groupThreadID.x, source, parameters);

    destination[texelIndex] = float4(color, 1.0);
}

#endif