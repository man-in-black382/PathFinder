#ifndef _DenoiserCommon__
#define _DenoiserCommon__

#include "Geometry.hlsl"
#include "ImportanceSampling.hlsl"

static const float MaxAccumulatedFrames = 16;
static const float MaxAccumulatedFramesInv = 1.0 / MaxAccumulatedFrames;

// https://developer.nvidia.com/gtc/2020/video/s22699

float GeometryWeight(float3 p0, float3 n0, float3 p, float p0ViewZ)
{
    // Weight based on distance from current sample "p"
    // and tangent plane to center sample "{p0, n0}"

    float frameAccumulationSpeed = 1.0;
    // Norm represents "1 / max possible allowed distance between a point and a plane"
    float norm = frameAccumulationSpeed / (1.0 + p0ViewZ);

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

#endif