#ifndef _Trig__
#define _Trig__

// From Nvidia CG documentation

// Returns the arccosine of a in the range[0, pi], expecting a to be in the range[-1, +1].
// Absolute error <= 6.7e-5
//
float AcosApprox(float x)
{
    float negate = float(x < 0);
    x = abs(x);
    float ret = -0.0187293;
    ret = ret * x;
    ret = ret + 0.0742610;
    ret = ret * x;
    ret = ret - 0.2121144;
    ret = ret * x;
    ret = ret + 1.5707288;
    ret = ret * sqrt(1.0 - x);
    ret = ret - 2 * negate * ret;
    return negate * 3.14159265358979 + ret;
}

// Returns the arcsine of a in the range[-pi / 2, +pi / 2], expecting a to be in the range[-1, +1].
//
float AsinApprox(float x) 
{
    float negate = float(x < 0);
    x = abs(x);
    float ret = -0.0187293;
    ret *= x;
    ret += 0.0742610;
    ret *= x;
    ret -= 0.2121144;
    ret *= x;
    ret += 1.5707288;
    ret = 3.14159265358979 * 0.5 - sqrt(1.0 - x) * ret;
    return ret - 2 * negate * ret;
}

// https://github.com/schuttejoe/Selas/blob/56a7fab5a479ec93d7f641bb64b8170f3b0d3095/Source/Core/MathLib/Trigonometric.h
//=========================================================================================================================
// These all assume vectors are in a z-up tangent space. Credit to PBRT for the idea to design these this way.
//=========================================================================================================================
float CosTheta(float3 w)
{
    return w.z;
}

float Cos2Theta(float3 w)
{
    return w.z * w.z;
}

float AbsCosTheta(float3 w)
{
    return abs(CosTheta(w));
}

float Sin2Theta(float3 w)
{
    return max(0.0f, 1.0f - Cos2Theta(w));
}

float SinTheta(float3 w)
{
    return sqrt(Sin2Theta(w));
}

float TanTheta(float3 w)
{
    return SinTheta(w) / CosTheta(w);
}

float Tan2Theta(float3 w)
{
    return Sin2Theta(w) / Cos2Theta(w);
}

float CosPhi(float3 w)
{
    float sinTheta = SinTheta(w);
    return (sinTheta == 0) ? 1.0f : clamp(w.x / sinTheta, -1.0f, 1.0f);
}

float SinPhi(float3 w)
{
    float sinTheta = SinTheta(w);
    return (sinTheta == 0) ? 1.0f : clamp(w.z / sinTheta, -1.0f, 1.0f);
}

float Cos2Phi(float3 w)
{
    float cosPhi = CosPhi(w);
    return cosPhi * cosPhi;
}

float Sin2Phi(float3 w)
{
    float sinPhi = SinPhi(w);
    return sinPhi * sinPhi;
}

#endif