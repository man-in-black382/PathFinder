#ifndef _HosekSky__
#define _HosekSky__

struct ArHosekSkyModelState
{
    float4 ConfigsX[3];
    float4 ConfigsY[3];
    float4 ConfigsZ[3];
    float4 Radiances;
};

struct PassData
{
    float3 SunDirection;
    uint SkyTexIdx;
    uint2 SkyTexSize;
    uint2 DispatchGroupCount;
    ArHosekSkyModelState SkyStateR; 
    ArHosekSkyModelState SkyStateG;
    ArHosekSkyModelState SkyStateB;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"
#include "Packing.hlsl"
#include "Light.hlsl"
#include "ColorConversion.hlsl"
#include "ThreadGroupTilingX.hlsl"

float ArHosekSkyModel_GetRadianceInternal(float4 config[3], float theta, float gamma)
{
    float configuration[9] = (float[9])config;

    const float expM = exp(configuration[4] * gamma);
    const float rayM = cos(gamma) * cos(gamma);
    const float mieM = (1.0 + cos(gamma) * cos(gamma)) / pow((1.0 + configuration[8] * configuration[8] - 2.0 * configuration[8] * cos(gamma)), 1.5);
    const float zenith = sqrt(cos(theta));

    return (1.0 + configuration[0] * exp(configuration[1] / (cos(theta) + 0.01))) *
        (configuration[2] + configuration[3] * expM + configuration[5] * rayM + configuration[6] * mieM + configuration[7] * zenith);
} 

static const uint GroupDimensionSize = 8;

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(uint3 groupThreadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
    RWTexture2D<float4> skyOutputTexture = RW_Float4_Textures2D[PassDataCB.SkyTexIdx];

    uint2 pixelIndex = ThreadGroupTilingX(PassDataCB.DispatchGroupCount, GroupDimensionSize.xx, 8, groupThreadID.xy, groupID.xy);
    float2 uv = TexelIndexToUV(pixelIndex, PassDataCB.SkyTexSize);

    float2 octVector = uv * 2.0 - 1.0;
    float3 sampleVector = OctDecode(octVector);
    
    float gamma = acos(dot(sampleVector, PassDataCB.SunDirection));
    // Clamp because hosek code is not robust against edge cases 
    float theta = acos(clamp(sampleVector.y, 0.0001, 0.9999));

    float r = ArHosekSkyModel_GetRadianceInternal(PassDataCB.SkyStateR.ConfigsX, theta, gamma) * PassDataCB.SkyStateR.Radiances[0];
    float g = ArHosekSkyModel_GetRadianceInternal(PassDataCB.SkyStateG.ConfigsY, theta, gamma) * PassDataCB.SkyStateG.Radiances[1];
    float b = ArHosekSkyModel_GetRadianceInternal(PassDataCB.SkyStateB.ConfigsZ, theta, gamma) * PassDataCB.SkyStateG.Radiances[2];

    float3 rgb = float3(r, g, b);
    rgb *= StandardLuminousEfficacy;

    skyOutputTexture[pixelIndex].rgb = rgb;
}


#endif