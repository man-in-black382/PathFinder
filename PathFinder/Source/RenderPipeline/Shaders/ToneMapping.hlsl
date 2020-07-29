#ifndef _ToneMappingRenderPass__
#define _ToneMappingRenderPass__

#include "GTTonemapping.hlsl"
#include "Exposure.hlsl"

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

[numthreads(32, 32, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    float2 uv = dispatchThreadID.xy * GlobalDataCB.PipelineRTResolutionInv;
    Texture2D inputImage = Textures2D[PassDataCB.InputTexIdx];
    RWTexture2D<float4> outputImage = RW_Float4_Textures2D[PassDataCB.OutputTexIdx];
    Texture2D normFactor = Textures2D[PassDataCB._Padding1];
    Texture2D gradient = Textures2D[PassDataCB._Padding0];

    /*   float3 shadowedShading = inputImage[dispatchThreadID.xy].rgb;
       float3 unshadowedShading = unshadowed[dispatchThreadID.xy].rgb;0
       float3 shadow = 0;

       [unroll] for (int i = 0; i < 3; ++i)
       {
           shadow[i] = unshadowedShading[i] < 1e-05 ? 1.0 : shadowedShading[i] / unshadowedShading[i];
       }*/

    float g = gradient[dispatchThreadID.xy].r / normFactor.SampleLevel(PointClampSampler, 0.xx, 11).r;
    // shadow* analytic[dispatchThreadID.xy].rgb;

    GTTonemappingParams params = PassDataCB.TonemappingParams;
    // Luminance was exposed using Saturation Based Sensitivity method 
    // hence the 1.0 for maximum luminance
    params.MaximumLuminance = 1.0;// ConvertEV100ToMaxHsbsLuminance(FrameDataCB.CurrentFrameCamera.ExposureValue100);

    //color = float3(
    //    GTToneMap(color.r, params),
    //    GTToneMap(color.g, params),
    //    GTToneMap(color.b, params));

    outputImage[dispatchThreadID.xy] = float4(/*SRGBFromLinear*/(g > 1.0 ? float3(0, 1, 0) : g.xxx), 1.0);
}

#endif