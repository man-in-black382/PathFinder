#ifndef _Exposure__
#define _Exposure__

#include "Utils.hlsl"
#include "Camera.hlsl"

// Physically-based camera from Moving Frostbite to PBR
// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf

float ComputeEV100(float aperture, float shutterTime, float ISO)
{
    // EV number is defined as:
    // 2^EV_s = N^2 / t and EV_s = EV_100 + log2 (S /100)
    // This gives
    // EV_s = log2 (N^2 / t)
    // EV_100 + log2 (S /100) = log2 (N^2 / t)
    // EV_100 = log2 (N^2 / t) - log2 (S /100)
    // EV_100 = log2 (N^2 / t * 100 / S)
    return log2(Square(aperture) / shutterTime * 100 / ISO);
}

float ComputeEV100FromAvgLuminance(float avgLuminance)
{
    // L_avg is the average scene luminance, S is the ISO arithmetic, and K is the reflected-light meter calibration constant.
    // ISO 2720:1974 recommends a range for K between 10.6 and 13.4.Two values for
    // K are in common use : 12.5 (Canon, Nikon, and Sekonic) and 14 (Minolta, Kenko, and Pentax)[Wikg].
    // K = 12.519 seems to be the most common adopted value among renderers.

    // We later use the middle gray at 12.7% in order to have
    // a middle gray at 18% with a sqrt (2) room for specular highlights
    // But here we deal with the spot meter measuring the middle gray
    // which is fixed at 12.5 for matching standard camera
    // constructor settings (i.e. calibration constant K = 12.5)

    return log2(avgLuminance * 100.0f / 12.5f);
}

float ComputeLuminousExposure(float aperture, float shutterTime)
{
    // Scene luminance reaching the sensor
    // L the incident luminance and q the lens and vignetting attenuation (a typical value is q = 0.65)
    // The actual value recorded by the sensor will depend on its sensitivity/gain
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
    float maxLuminance = 1.2f * pow(2.0f, EV100);

    return maxLuminance;
}

float3 ExposeLuminance(float3 luminance, Camera camera)
{
    float maxLuminance = ConvertEV100ToMaxHsbsLuminance(camera.ExposureValue100);
    return luminance / maxLuminance;
}

#endif