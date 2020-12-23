#ifndef _SphericalHarmonics__
#define _SphericalHarmonics__

#include "Constants.hlsl"

static const float Y00 = 0.28209479177387814347f; // 1 / (2*sqrt(pi))
static const float Y11 = -0.48860251190291992159f; // sqrt(3 /(4pi))
static const float Y10 = 0.48860251190291992159f;
static const float Y1_1 = -0.48860251190291992159f;
static const float Y21 = -1.09254843059207907054f; // 1 / (2*sqrt(pi))
static const float Y2_1 = -1.09254843059207907054f;
static const float Y2_2 = 1.09254843059207907054f;
static const float Y20 = 0.31539156525252000603f; // 1/4 * sqrt(5/pi)
static const float Y22 = 0.54627421529603953527f; // 1/4 * sqrt(15/pi)

// https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
//
static const float CosineLobeBandFactors[] = {
        Pi,
        2.0f * Pi / 3.0f, 2.0f * Pi / 3.0f, 2.0f * Pi / 3.0f,
        Pi / 4.0f, Pi / 4.0f, Pi / 4.0f, Pi / 4.0f, Pi / 4.0f
};

struct SH3_RGB
{
    float3 L00;
    float3 L11;
    float3 L10;
    float3 L1_1;
    float3 L21;
    float3 L2_1;
    float3 L2_2;
    float3 L20;
    float3 L22;
};

SH3_RGB ZeroSH()
{
    SH3_RGB sh;
    sh.L00 = 0.0;
    sh.L11 = 0.0;
    sh.L10 = 0.0;
    sh.L1_1 = 0.0;
    sh.L21 = 0.0;
    sh.L2_1 = 0.0;
    sh.L2_2 = 0.0;
    sh.L20 = 0.0;
    sh.L22 = 0.0;
    return sh;
}

void ProjectOnSH(inout SH3_RGB sh, float3 direction, float3 value, float weight)
{
    // l, m = 0, 0
    sh.L00 += value * Y00 * CosineLobeBandFactors[0] * weight;

    // l, m = 1, -1
    sh.L1_1 += value * Y1_1 * CosineLobeBandFactors[1] * direction.y * weight;
    // l, m = 1, 0
    sh.L10 += value * Y10 * CosineLobeBandFactors[2] * direction.z * weight;
    // l, m = 1, 1
    sh.L11 += value * Y11 * CosineLobeBandFactors[3] * direction.x * weight;

    // l, m = 2, -2
    sh.L2_2 += value * Y2_2 * CosineLobeBandFactors[4] * (direction.x * direction.y) * weight;
    // l, m = 2, -1
    sh.L2_1 += value * Y2_1 * CosineLobeBandFactors[5] * (direction.y * direction.z) * weight;
    // l, m = 2, 1
    sh.L21 += value * Y21 * CosineLobeBandFactors[6] * (direction.x * direction.z) * weight;

    // l, m = 2, 0
    sh.L20 += value * Y20 * CosineLobeBandFactors[7] * (3.0f * direction.z * direction.z - 1.0f) * weight;

    // l, m = 2, 2
    sh.L22 += value * Y22 * CosineLobeBandFactors[8] * (direction.x * direction.x - direction.y * direction.y) * weight;
}

SH3_RGB ScaleSH(SH3_RGB sh, float3 scale)
{
    SH3_RGB result;

    result.L00 = scale * sh.L00;

    result.L1_1 = scale * sh.L1_1;
    result.L10 = scale * sh.L10;
    result.L11 = scale * sh.L11;

    result.L2_2 = scale * sh.L2_2;
    result.L2_1 = scale * sh.L2_1;
    result.L21 = scale * sh.L21;

    result.L20 = scale * sh.L20;

    result.L22 = scale * sh.L22;

    return result;
}

SH3_RGB SumSH(SH3_RGB first, SH3_RGB second)
{
    SH3_RGB result;

    result.L00 = first.L00 + second.L00;

    result.L1_1 = first.L1_1 + second.L1_1;
    result.L10 = first.L10 + second.L10;
    result.L11 = first.L11 + second.L11;

    result.L2_2 = first.L2_2 + second.L2_2;
    result.L2_1 = first.L2_1 + second.L2_1;
    result.L21 = first.L21 + second.L21;

    result.L20 = first.L20 + second.L20;

    result.L22 = first.L22 + second.L22;

    return result;
}

SH3_RGB SumSH(SH3_RGB first, SH3_RGB second, SH3_RGB third, SH3_RGB fourth, SH3_RGB fifth, SH3_RGB sixth, SH3_RGB seventh, SH3_RGB eighth) 
{
    SH3_RGB result;

    result.L00 = first.L00 + second.L00 + third.L00 + fourth.L00 + fifth.L00 + sixth.L00 + seventh.L00 + eighth.L00;
    result.L1_1 = first.L1_1 + second.L1_1 + third.L1_1 + fourth.L1_1 + fifth.L1_1 + sixth.L1_1 + seventh.L1_1 + eighth.L1_1;
    result.L10 = first.L10 + second.L10 + third.L10 + fourth.L10 + fifth.L10 + sixth.L10 + seventh.L10 + eighth.L10;
    result.L11 = first.L11 + second.L11 + third.L11 + fourth.L11 + fifth.L11 + sixth.L11 + seventh.L11 + eighth.L11;
    result.L2_2 = first.L2_2 + second.L2_2 + third.L2_2 + fourth.L2_2 + fifth.L2_2 + sixth.L2_2 + seventh.L2_2 + eighth.L2_2;
    result.L2_1 = first.L2_1 + second.L2_1 + third.L2_1 + fourth.L2_1 + fifth.L2_1 + sixth.L2_1 + seventh.L2_1 + eighth.L2_1;
    result.L21 = first.L21 + second.L21 + third.L21 + fourth.L21 + fifth.L21 + sixth.L21 + seventh.L21 + eighth.L21;
    result.L20 = first.L20 + second.L20 + third.L20 + fourth.L20 + fifth.L20 + sixth.L20 + seventh.L20 + eighth.L20;
    result.L22 = first.L22 + second.L22 + third.L22 + fourth.L22 + fifth.L22 + sixth.L22 + seventh.L22 + eighth.L22;

    return result;
}

float SHMagnitude2(SH3_RGB sh)
{
    return dot(sh.L00, sh.L00) +

        dot(sh.L1_1, sh.L1_1) +
        dot(sh.L10, sh.L10) +
        dot(sh.L11, sh.L11) +

        dot(sh.L2_2, sh.L2_2) +
        dot(sh.L2_1, sh.L2_1) +
        dot(sh.L20, sh.L20) +
        dot(sh.L21, sh.L21) +
        dot(sh.L22, sh.L22);
}

float3 EvaluateSH(SH3_RGB sh, float3 direction)
{
    float3 result = 0.0;

    result += sh.L00 * Y00;

    result += sh.L1_1 * Y1_1 * direction.y;
    result += sh.L10 * Y10 * direction.z;
    result += sh.L11 * Y11 * direction.x;

    result += sh.L2_2 * Y2_2 * (direction.x * direction.y);
    result += sh.L2_1 * Y2_1 * (direction.y * direction.z);
    result += sh.L21 * Y21 * (direction.x * direction.z);
    result += sh.L20 * Y20 * (3.0f * direction.z * direction.z - 1.0f);
    result += sh.L22 * Y22 * (direction.x * direction.x - direction.y * direction.y);

    return result;
}

#endif