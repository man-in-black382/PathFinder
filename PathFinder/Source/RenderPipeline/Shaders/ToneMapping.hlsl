#ifndef _ToneMappingRenderPass__
#define _ToneMappingRenderPass__

#include "GTTonemapping.hlsl"
#include "Exposure.hlsl"
#include "GBuffer.hlsl"

struct PassData
{
    uint InputTexIdx;
    uint OutputTexIdx;
    uint _Padding0;
    uint _Padding1;
    // 16 byte boundary
    GTTonemappingParams TonemappingParams;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"
#include "ColorConversion.hlsl"

[numthreads(16, 16, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    Texture2D inputImage = Textures2D[PassDataCB.InputTexIdx];
    RWTexture2D<float4> outputImage = RW_Float4_Textures2D[PassDataCB.OutputTexIdx];

    float3 color = inputImage.mips[0][dispatchThreadID.xy].rgb;

    GTTonemappingParams params = PassDataCB.TonemappingParams;
    // Luminance was exposed using Saturation Based Sensitivity method 
    // hence the 1.0 for maximum luminance
    params.MaximumLuminance = 1.0;// ConvertEV100ToMaxHsbsLuminance(FrameDataCB.CurrentFrameCamera.ExposureValue100);

    color = float3(
        GTToneMap(color.r, params),
        GTToneMap(color.g, params),
        GTToneMap(color.b, params));

    outputImage[dispatchThreadID.xy] = float4(color, 1.0);
}

#endif