struct PassData
{
    uint InputTextureIndex;
    uint OutputTextureIndex;
};

#define PassDataType PassData

#include "BaseRootSignature.hlsl"
#include "ColorConversion.hlsl"

// Constants

// Functions
// https://github.com/Zackin5/Filmic-Tonemapping-ReShade/blob/master/Uncharted2.fx

// Should be in range [0.0; 1.0]
static const float ShoulderStrength = 0.22;

// Should be in range [0.0; 1.0]
static const float LinearStrength = 0.30;

// Should be in range [0.0; 1.0]
static const float LinearAngle = 0.10;

// Should be in range [0.0; 1.0]
static const float ToeStrength = 0.20;

// Should be in range [0.0; 1.0]
static const float ToeNumerator = 0.20;

// Should be in range [0.0; 1.0]
static const float ToeDenominator = 0.22;

// Should be in range [0.0; 20.0]
static const float LinearWhitePoint = 11.2;

static const float ExposureBias = 2.0;

static const float ExposureNominator = 16.0;

float3 Uncharted2Tonemap(float x)
{
    return ((x * (ShoulderStrength * x + LinearAngle * LinearStrength) + ToeStrength * ToeNumerator)
        / (x * (ShoulderStrength * x + LinearStrength) + ToeStrength * ToeDenominator)) - ToeNumerator / ToeDenominator;
}

float3 Reinhard(float3 v)
{
    return v / (1.0f + v);
}

[numthreads(32, 32, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    Texture2D inputImage = Textures2D[PassDataCB.InputTextureIndex];
    RWTexture2D<float4> outputImage = RW_Float4_Textures2D[PassDataCB.OutputTextureIndex];
    
    uint3 loadCoords = uint3(dispatchThreadID.xy, 0);

    //    float exposure = texelFetch(uExposure, ivec2(0), 0).r;

    float3 color = inputImage.Load(loadCoords).rgb;


    outputImage[dispatchThreadID.xy] = float4(color, 1.0);
    return;



    color *= 0.003;//exposure;  // Exposure Adjustment

    float lum = Luminance(color);
    float exposedLuminance = ExposureBias * lum;

    float3 newLum = Uncharted2Tonemap(exposedLuminance);
    float3 lumScale = newLum / lum;
    color *= lumScale;

    float3 whiteScale = 1.0f / Uncharted2Tonemap(LinearWhitePoint);
    color *= whiteScale;

    outputImage[dispatchThreadID.xy] = float4(SRGBFromLinear(color), 1.0);
}