#ifndef _Random__
#define _Random__

// https://stackoverflow.com/a/17479300
// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint Hash(uint x) 
{
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

// Compound versions of the hashing algorithm I whipped together.
uint Hash(uint2 v) { return Hash(v.x ^ Hash(v.y)); }
uint Hash(uint3 v) { return Hash(v.x ^ Hash(v.y) ^ Hash(v.z)); }
uint Hash(uint4 v) { return Hash(v.x ^ Hash(v.y) ^ Hash(v.z) ^ Hash(v.w)); }

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float FloatConstruct(uint m)
{
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa; // Keep only mantissa bits (fractional part)
    m |= ieeeOne; // Add fractional part to 1.0

    float  f = asfloat(m); // Range [1:2]
    return f - 1.0; // Range [0:1]
}

// Pseudo-random value in half-open range [0:1].
float Random(float x)  { return FloatConstruct(Hash(asuint(x))); }
float Random(float2 v) { return FloatConstruct(Hash(asuint(v))); }
float Random(float3 v) { return FloatConstruct(Hash(asuint(v))); }
float Random(float4 v) { return FloatConstruct(Hash(asuint(v))); }

// https://www.gamedev.net/articles/programming/graphics/contact-hardening-soft-shadows-made-fast-r4906/
//
float2 VogelDiskSample(int sampleIndex, int samplesCount, float phi) 
{
    static const float GoldenAngle = 2.4f;

    float r = sqrt(float(sampleIndex) + 0.5f) / sqrt(float(samplesCount));
    float theta = float(sampleIndex) * GoldenAngle + phi;

    float sine;
    float cosine;

    sincos(theta, sine, cosine);

    return float2(r * cosine, r * sine);
}

// Hammersley Points on the Hemisphere
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Generates a uniformly distributed and stochastic-looking sampling pattern at low computational cost
// Used for quasi-Monte Carlo integration
float2 Hammersley2D(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

#endif