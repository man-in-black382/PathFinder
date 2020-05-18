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

#endif