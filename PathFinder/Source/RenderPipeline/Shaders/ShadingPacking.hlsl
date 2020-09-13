#ifndef _ShadingPacking__
#define _ShadingPacking__

#include "ColorConversion.hlsl"
#include "Light.hlsl"
#include "Packing.hlsl"
#include "Utils.hlsl"

static const uint YBitCount = 16;
static const uint CoBitCount = 8;
static const uint CgBitCount = 8;

static const uint AllBits1 = U32Max;
static const uint YMask = AllBits1 >> (32 - YBitCount);
static const uint CoMask = AllBits1 >> (32 - CoBitCount);
static const uint CgMask = AllBits1 >> (32 - CgBitCount);

struct RTData
{
    // 4 components for 4 ray-light pairs
    uint4 BRDFResponses;
    uint4 RayLightIntersectionData;
    float4 ShadowFactors;
};

RTData ZeroRTData()
{
    RTData data;
    data.BRDFResponses = 0.xxxx;
    data.RayLightIntersectionData = 0.xxxx;
    data.ShadowFactors = 1.xxxx;
    return data;
}

//--------------------------------------------------------------------------------------------------
// Functions to pack and unpack data necessary for shadows ray tracing pass.
// Avoids using arrays and allows us to pack everything into single 4-component registers to avoid 
// register spilling.
//--------------------------------------------------------------------------------------------------
void SetStochasticBRDFMagnitude(inout RTData rtData, float3 brdf, uint rayLightPairIndex)
{
    // Leverage YCoCg compressibility
    float3 ycocg = RGBToYCoCg(brdf);
    float magnitude = 1.0;

    uint y = PackSnorm(ycocg.x, magnitude, YBitCount);
    uint co = PackSnorm(ycocg.y, magnitude, CoBitCount);
    uint cg = PackSnorm(ycocg.z, magnitude, CgBitCount);

    uint packed = 0;
    packed |= y;
    packed |= (co << YBitCount);
    packed |= (cg << (YBitCount + CoBitCount));

    rtData.BRDFResponses[rayLightPairIndex] = packed;
}

float3 GetStochasticBRDFMagnitude(RTData rtData, uint rayLightPairIndex)
{
    uint packed = rtData.BRDFResponses[rayLightPairIndex];
    float magnitude = 1.0;

    uint yPacked = packed & YMask;
    uint coPacked = (packed >> YBitCount) & CoMask;
    uint cgPacked = (packed >> (YBitCount + CoBitCount)) & CgMask;

    float3 ycocg = float3(
        UnpackSnorm(yPacked, magnitude, YBitCount),
        UnpackSnorm(coPacked, magnitude, CoBitCount),
        UnpackSnorm(cgPacked, magnitude, CgBitCount));

    float3 rgb = YCoCgToRGB(ycocg);

    return rgb;
}

void SetRaySphericalLightIntersectionPoint(inout RTData rtData, Light light, float3 interectionPoint, uint rayLightPairIndex)
{
    // For spherical lights we encode a normalized direction from light's center to intersection point
    float3 lightToIntersection = normalize(interectionPoint - light.Position.xyz);

    rtData.RayLightIntersectionData[rayLightPairIndex] = EncodeNormalSignedOct(lightToIntersection);
}

float3 GetRaySphericalLightIntersectionPoint(RTData rtData, Light light, uint rayLightPairIndex)
{
    float3 lightToIntersection = DecodeNormalSignedOct(rtData.RayLightIntersectionData[rayLightPairIndex]);
    return lightToIntersection * light.Width * 0.5 + light.Position.xyz;
}

void SetRayRectangularLightIntersectionPoint(inout RTData rtData, Light light, float3x3 lightRotation, float3 interectionPoint, uint rayLightPairIndex)
{
    // Translate point to light's local coordinate system
    float3 localPoint = interectionPoint - light.Position.xyz;
    // Rotate point back so it's positioned on a rectangle perpendicular to Z axis
    float3 zAlighnedPoint = mul(transpose(lightRotation), localPoint);
    // Normalize xy to get uv of the intersection point
    float2 uv = zAlighnedPoint.xy / float2(light.Width, light.Height) + 0.5;

    rtData.RayLightIntersectionData[rayLightPairIndex] = PackUnorm2x16(uv.x, uv.y, 1.0);
}

float3 GetRayRectangularLightIntersectionPoint(RTData rtData, Light light, float3x3 lightRotation, uint rayLightPairIndex)
{
    float2 uv = UnpackUnorm2x16(rtData.RayLightIntersectionData[rayLightPairIndex], 1.0);
    float2 localXY = (uv - 0.5) * float2(light.Width, light.Height);
    float3 localRotated = mul(lightRotation, float3(localXY, 0.0));
    float3 worldPoint = localRotated + light.Position.xyz;

    return worldPoint;
}

void SetRayDiskLightIntersectionPoint(inout RTData rtData, Light light, float3x3 lightRotation, float3 interectionPoint, uint rayLightPairIndex)
{
    // Code for rect light can be reused here
    SetRayRectangularLightIntersectionPoint(rtData, light, lightRotation, interectionPoint, rayLightPairIndex);
}

float3 GetRayDiskLightIntersectionPoint(RTData rtData, Light light, float3x3 lightRotation, uint rayLightPairIndex)
{
    return GetRayRectangularLightIntersectionPoint(rtData, light, lightRotation, rayLightPairIndex);
}

#endif