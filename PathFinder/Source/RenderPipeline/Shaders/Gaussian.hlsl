#ifndef _Gaussian__
#define _Gaussian__

#include "Constants.hlsl"

// Incremental Gaussian Coefficent Calculation (See GPU Gems 3 pp. 877 - 889)
struct IncrementalGaussian
{
    float3 V;
};

IncrementalGaussian GaussianApproximation(float sigma)
{
    IncrementalGaussian ig;
    ig.V.x = 1.0 / (sqrt(2.0 * Pi) * sigma);
    ig.V.y = exp(-0.5 / (sigma * sigma));
    ig.V.z = ig.V.y * ig.V.y;
    return ig;
}

float IncrementGaussian(inout IncrementalGaussian ig)
{
    ig.V.xy *= ig.V.yz;
    return ig.V.x;
}

float GetIncrementalGaussianWeight(IncrementalGaussian ig)
{
    return ig.V.x;
}

#endif