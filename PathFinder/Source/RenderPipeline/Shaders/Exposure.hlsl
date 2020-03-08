#ifndef _Exposure__
#define _Exposure__

#include "Utils.hlsl"

// Physically-based camera from Moving Frostbite to PBR
// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf

float ComputeEV100(float aperture, float shutterTime, float ISO)
{
    // EV number is defined as:
    // 2^ EV_s = N^2 / t and EV_s = EV_100 + log2 (S /100)
    // This gives
    // EV_s = log2 (N^2 / t)
    // EV_100 + log2 (S /100) = log2 (N^2 / t)
    // EV_100 = log2 (N^2 / t) - log2 (S /100)
    // EV_100 = log2 (N^2 / t . 100 / S)
    return log2(Square(aperture) / shutterTime * 100 / ISO);
}

float ComputeEV100FromAvgLuminance(float avgLuminance)
{
    // We later use the middle gray at 12.7% in order to have
    // a middle gray at 18% with a sqrt (2) room for specular highlights
    // But here we deal with the spot meter measuring the middle gray
    // which is fixed at 12.5 for matching standard camera
    // constructor settings (i.e. calibration constant K = 12.5)
    // Reference: http://en.wikipedia.org/wiki/Film_speed
    return log2(avgLuminance * 100.0f / 12.5f);
}

float ComputeLuminousExposure(float aperture, float shutterTime)
{
    const float q = 0.65;
    float H = q * shutterTime / Square(aperture);
    return H;
}

float ConvertEV100ToMaxHsbsLuminance(float EV100)
{
    // Compute the maximum luminance possible with H_sbs sensitivity.
    // Saturation Based Sensitivity: defined as the maximum possible exposure that does
    // not lead to a clipped or bloomed camera output

    // maxLum = 78 / ( S * q ) * N^2 / t
    // = 78 / ( S * q ) * 2^ EV_100
    // = 78 / (100 * 0.65) * 2^ EV_100
    // = 1.2 * 2^ EV
    // Reference: http://en.wikipedia.org/wiki/Film_speed
    float maxLuminance = 1.2f * pow(2.0f, EV100);

    return maxLuminance;
}

float ConvertEV100ToExposure(float EV100)
{
    float maxLuminance = ConvertEV100ToMaxHsbsLuminance(EV100);
    return 1.0f / maxLuminance;
}

#endif