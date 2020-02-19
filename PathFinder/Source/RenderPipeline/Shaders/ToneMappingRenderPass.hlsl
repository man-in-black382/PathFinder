#include "GTTonemapping.hlsl"

struct PassData
{
    uint InputTextureIndex;
    uint OutputTextureIndex;
    GTTonemappingParams TonemappingParams;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"
#include "ColorConversion.hlsl"

[numthreads(32, 32, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    Texture2D inputImage = Textures2D[PassDataCB.InputTextureIndex];
    RWTexture2D<float4> outputImage = RW_Float4_Textures2D[PassDataCB.OutputTextureIndex];
    
    uint3 loadCoords = uint3(dispatchThreadID.xy, 0);
    float3 color = 0.0.xxx;// inputImage.Load(loadCoords).rgb;

    float3 tonemappedColor = float3(
        GTToneMap(color.r, PassDataCB.TonemappingParams),
        GTToneMap(color.g, PassDataCB.TonemappingParams),
        GTToneMap(color.b, PassDataCB.TonemappingParams)
    );

    float x = color.r;

    float P = PassDataCB.TonemappingParams.MaximumLuminance;
    float a = PassDataCB.TonemappingParams.Contrast;
    float m = PassDataCB.TonemappingParams.LinearSectionStart;
    float l = PassDataCB.TonemappingParams.LinearSectionLength;
    float c = PassDataCB.TonemappingParams.BlackTightness;
    float b = PassDataCB.TonemappingParams.MinimumBrightness;
    float l0 = (P - m) * l / a;
    float L0 = m - m / a;
    float L1 = m + (1 - m) / a;
    float Lx = m + a * (x - m); // Linear part
    float Tx = m > 1e-5 ? m * pow(abs(x / m), c) + b : b; // Toe
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = a * P / (P - S1);
    float Sx = P - (P - S1) * exp(-C2 * (x - S0) / P); // Shoulder
    float w0x = 1.0 - GTW(x, 0.0, m);
    float w2x = x > (m + l0) ? 1 : 0;
    float w1x = 1.0 - w0x - w2x;
    float debugC = (Tx * w0x + Lx * w1x + Sx * w2x);

    //tonemappedColor = color;

    outputImage[dispatchThreadID.xy] = float4(SRGBFromLinear(tonemappedColor), 1.0);
}