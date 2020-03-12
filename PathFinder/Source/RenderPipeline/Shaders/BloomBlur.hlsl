#ifndef _BloomBlur__
#define _BloomBlur__

#include "GaussianBlur.hlsl"

struct PassCBData
{
    // Packing into 4-component vectors 
    // to satisfy constant buffer alignment rules
    float4 Weights[GaussianBlurMaximumRadius / 4]; 
};

struct RootConstants
{
    float2 ImageSize;
    uint IsHorizontal;
    uint BlurRadius;
    uint InputTextureIndex;
    uint OutputTextureIndex;
};

#define PassDataType PassCBData

#include "MandatoryEntryPointInclude.hlsl"

ConstantBuffer<RootConstants> RootConstantBuffer : register(b0);

[numthreads(GaussianBlurGroupSize, 1, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    RWTexture2D<float4> source = RW_Float4_Textures2D[RootConstantBuffer.InputTextureIndex];
    RWTexture2D<float4> destination = RW_Float4_Textures2D[RootConstantBuffer.OutputTextureIndex];

    GaussianBlurParameters parameters;
    parameters.Weights = PassDataCB.Weights;
    parameters.ImageSize = RootConstantBuffer.ImageSize;
    parameters.IsHorizontal = false;// RootConstantBuffer.IsHorizontal;
    parameters.BlurRadius = RootConstantBuffer.BlurRadius;

    float3 color = BlurGaussian(dispatchThreadID.yx, groupThreadID.xy, source, parameters);

    destination[dispatchThreadID.xy] = float4(color, 1.0); 
}

#endif