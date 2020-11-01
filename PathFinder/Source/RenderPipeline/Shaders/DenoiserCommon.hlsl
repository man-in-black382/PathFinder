#ifndef _DenoiserCommon__
#define _DenoiserCommon__

#include "Geometry.hlsl"
#include "ImportanceSampling.hlsl"

static const float MaxAccumulatedFrames = 16;
static const float MaxFrameCountWithHistoryFix = 4;
static const float MaxAccumulatedFramesInv = 1.0 / MaxAccumulatedFrames;
static const float MaxFrameCountWithHistoryFixInv = 1.0 / MaxFrameCountWithHistoryFix;

static const uint StrataPositionPackingMask = 7; // 0b111
static const uint StrataPositionPackingShift = 3; // 3 bits
static const uint GradientUpscaleCoefficient = 3;
static const float GradientUpscaleCoefficientInv = 1.0 / GradientUpscaleCoefficient;

uint PackStratumPosition(uint2 position)
{
    uint packed = 0;
    packed = (position.y & StrataPositionPackingMask) << StrataPositionPackingShift;
    packed |= position.x & StrataPositionPackingMask;
    return packed;
}

uint2 UnpackStratumPosition(uint packed)
{
    return uint2(packed & StrataPositionPackingMask, packed >> StrataPositionPackingShift);
}

// https://developer.nvidia.com/gtc/2020/video/s22699

float GeometryWeight(float3 p0, float3 n0, float3 p)
{
    // Weight based on distance from current sample "p"
    // and tangent plane to center sample "{p0, n0}"

    // https://www.desmos.com/calculator/qqtksm0fv2

    const float Sensitivity = 20;
    // Norm represents "1 / max possible allowed distance between a point and a plane"
    float norm = Sensitivity / (1.0 + p0.z);

    Plane plane = InitPlane(n0, p0);
    float distanceToPlane = PointDistanceToPlane(p, plane);
    float weight = saturate(1.0 - abs(distanceToPlane) * norm);

    return weight;
}

float NormalWeight(float3 n0, float3 n, float roughness, float accumulatedFrameCount)
{
    // Rejects samples if the angle between two normals is higher than the specular lobe half angle
    float a0 = GGXLobeHalfAngle(roughness);

    // Close to zero if accumulation goes well
    // Make the angle more narrow over time
    float accumulationFactor = 1.0 - accumulatedFrameCount * MaxAccumulatedFramesInv;
    a0 = a0 * accumulationFactor + radians(0.5);

    float cosa = saturate(dot(n0, n));
    float a = acos(cosa);

    // 0 if angle is too large or a linear remapping of the angle to [0;1]
    // Remapping: 1 when angle is 0 and 0 when angle approaches half GGX lobe angle
    float weight = a > a0 ? 0.0 : (1.0 - a / a0); 

    return weight; 
}

float RoughnessWeight(float roughness0, float roughness)
{
    float norm = roughness0 * roughness0 * 0.99 + 0.01;
    float weight = abs(roughness0 - roughness) / norm;

    return saturate(1.0 - weight);
}

// Gradient for HF and SPEC channels is computed as the relative difference between
// path tracer outputs on the current and previous frame, for a given gradient pixel. 
float GetHFGradient(float currLuminance, float prevLuminance)
{
    float maxLuminance = max(currLuminance, prevLuminance);

    // Prev. lum. is negative when we left a hole during reprojection
    if (maxLuminance == 0 || prevLuminance < 0.0)
    {
        return 0.0;
    }

    float gradient = abs(currLuminance - prevLuminance) / maxLuminance;
    gradient *= gradient; // Make small changes less significant

    return gradient;
}

float3 CombineShading(float3 analytic, float3 stochasticShadowed, float3 stochasticUnshadowed)
{
    float3 shadow = 0;

    [unroll] 
    for (int i = 0; i < 3; ++i)
    {
        shadow[i] = stochasticUnshadowed[i] < 1e-05 ? 1.0 : stochasticShadowed[i] / stochasticUnshadowed[i];
    }

    return analytic * shadow;
}

#endif