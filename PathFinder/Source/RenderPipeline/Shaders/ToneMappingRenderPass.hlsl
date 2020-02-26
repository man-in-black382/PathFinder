#include "GTTonemapping.hlsl"

struct PassData
{
    uint InputTextureIndex;
    uint OutputTextureIndex;
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
    Texture2D inputImage = Textures2D[PassDataCB.InputTextureIndex];
    RWTexture2D<float4> outputImage = RW_Float4_Textures2D[PassDataCB.OutputTextureIndex];
    
    uint3 loadCoords = uint3(dispatchThreadID.xy, 0);
    float3 color = inputImage.Load(loadCoords).rgb;

    float3 tonemappedColor = float3(
        GTToneMap(color.r, PassDataCB.TonemappingParams),
        GTToneMap(color.g, PassDataCB.TonemappingParams),
        GTToneMap(color.b, PassDataCB.TonemappingParams)
    );

    outputImage[dispatchThreadID.xy] = float4(SRGBFromLinear(tonemappedColor), 1.0);
}