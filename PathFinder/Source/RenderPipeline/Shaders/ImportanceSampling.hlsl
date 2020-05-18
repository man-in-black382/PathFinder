#ifndef _ImportanceSampling__
#define _ImportanceSampling__

#include "Constants.hlsl"

// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
// Page 69
//
float3 GGXDominantDirection(float3 N, float3 V, float roughness)
{
    float smoothness = 1.0 - roughness;
    float f = smoothness * (sqrt(smoothness) + roughness);
    float3 R = reflect(-V, N);
    float3 direction = lerp(N, R, f);

    return direction;
}

float GGXLobeHalfAngle(float roughness)
{
    float roughness2 = roughness * roughness;
    float angle = (90.0 * roughness2) / (1.0 + roughness2);
    return angle * PiOver180;
}

#endif