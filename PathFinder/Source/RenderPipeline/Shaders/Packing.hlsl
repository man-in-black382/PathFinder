#ifndef _Packing__
#define _Packing__

#include "Constants.hlsl"
#include "Utils.hlsl"

uint PackUnorm(float value, float valueRange, uint bitCount)
{
    uint base = (1u << (bitCount - 1)) - 1u;
    float valueNorm = abs(value) / valueRange;
    uint packed = uint(valueNorm * base);
    return packed;
}

float UnpackUnorm(uint packed, float valueRange, uint bitCount)
{
    uint base = (1u << (bitCount - 1)) - 1u;
    float valueNorm = float(packed) / float(base);
    float value = valueNorm * valueRange;
    return value;
}

uint PackSnorm(float value, float valueRange, uint bitCount)
{
    uint base = (1u << (bitCount - 2)) - 1u; // bit count - 2 to leave 1 bit for sign
    uint signBit = uint(saturate(Sign(value))) << (bitCount - 1); // 1 for positive, 0 for negative
    float valueNorm = abs(value) / valueRange;
    uint packed = uint(valueNorm * base);
    packed |= signBit;

    return packed;
}

float UnpackSnorm(uint packed, float valueRange, uint bitCount)
{
    uint highestBit = 1u << (bitCount - 1);
    uint base = highestBit - 1u; // Highest bit contains sigh, hence the -1 to determine base value
    uint signBit = packed & highestBit;
    float sign = signBit > 0 ? 1.0 : -1.0;
    float valueNormUnsigned = float(packed & ~highestBit) / base; // Get rid of the sign bit then normalize
    float valueNorm = valueNormUnsigned * sign;
    float value = valueNorm * valueRange;
    
    return value;
}

uint PackSnorm2x16(float first, float second, float range)
{
    static const float base = 32767.0;

    float rangeInverse = 1.0 / range;

    uint packed = 0;
    uint iFirst = uint(int(first * rangeInverse * base));
    uint iSecond = uint(int(second * rangeInverse * base));

    uint firstSignMask = iFirst & (1u << 31); // Leave only sign bit

    uint secondSignMask = iSecond & (1u << 31); // Leave only sign bit
    secondSignMask >>= 16; // Move sign mask by 16 since second value will be stored in LSB of the final uint

    // Move uFirst into MS bits
    packed |= iFirst;
    packed <<= 16;
    packed |= firstSignMask; // Set sign bit

    // Move uSecond into LS bits
    packed |= iSecond & 0x0000FFFFu;
    packed |= secondSignMask; // Set sign bit

    return packed;
}

float2 UnpackSnorm2x16(uint package, float range)
{
    static const float baseInverse = 1.0 / 32767.0;

    // Unpack encoded floats into individual variables
    uint uFirst = package >> 16;
    uint uSecond = package & 0x0000FFFFu;

    // Extract sign bits
    uint firstSignMask = uFirst & (1u << 15);
    uint secondSignMask = uSecond & (1u << 15);

    // If sign bit indicates negativity, fill MS 16 bits with 1s
    uFirst |= firstSignMask != 0 ? 0xFFFF0000u : 0x0u;
    uSecond |= secondSignMask != 0 ? 0xFFFF0000u : 0x0u;

    // Now get signed integer representation
    int iFirst = int(uFirst);
    int iSecond = int(uSecond);

    // At last, convert integers back to floats using range and base
    float fFirst = (float(iFirst) * baseInverse) * range;
    float fSecond = (float(iSecond) * baseInverse) * range;

    return float2(fFirst, fSecond);
}

uint PackUnorm2x16(float first, float second, float range)
{
    static const float base = 65535.0;

    float rangeInverse = 1.0 / range;

    uint packed = 0;
    uint iFirst = uint(abs(first) * rangeInverse * base);
    uint iSecond = uint(abs(second) * rangeInverse * base);

    packed |= iFirst;
    packed <<= 16;

    packed |= (iSecond & 0x0000FFFFu);

    return packed;
}

float2 UnpackUnorm2x16(uint package, float range)
{
    static const float baseInverse = 1.0 / 65535.0;

    // Unpack encoded floats into individual variables
    uint uFirst = package >> 16;
    uint uSecond = package & 0x0000FFFFu;

    // At last, convert integers back to floats using range and base
    float fFirst = (float(uFirst) * baseInverse) * range;
    float fSecond = (float(uSecond) * baseInverse) * range;

    return float2(fFirst, fSecond);
}

float4 Decode8888(uint encoded)
{
    float4 decoded;
    decoded.x = (0xFF000000u & encoded) >> 24;
    decoded.y = (0x00FF0000u & encoded) >> 16;
    decoded.z = (0x0000FF00u & encoded) >> 8;
    decoded.w = (0x000000FFu & encoded);
    decoded /= 255.0;
    return decoded;
}

uint Encode8888(float4 vec)
{
    uint rgba = (uint(vec.x * 255.0) << 24) |
    (uint(vec.y * 255.0) << 16) |
    (uint(vec.z * 255.0) << 8) |
    uint(vec.w * 255.0);
    return rgba;
}

// Signed Octahedron Normal Encoding
// http://johnwhite3d.blogspot.com/2017/10/signed-octahedron-normal-encoding.html
//
uint EncodeNormalSignedOct(float3 n)
{
    float3 outN;

    n /= (abs(n.x) + abs(n.y) + abs(n.z));

    outN.y = n.y * 0.5 + 0.5;
    outN.x = n.x * 0.5 + outN.y;
    outN.y = n.x * -0.5 + outN.y;

    outN.z = saturate(n.z * FloatMax);

    uint signBit = outN.z > 0.0 ? 1 : 0;
    uint encoded = PackUnorm2x16(outN.x, outN.y, 1.0);

    // Sacrifice LS bit of one of the encoded 
    // values to store normal z sign
    encoded |= signBit;

    return encoded;
}

float3 DecodeNormalSignedOct(uint encoded)
{
    float3 n;
    // Extract sign and convert it to float
    n.z = float(encoded & 1u);
    n.xy = UnpackUnorm2x16(encoded, 1.0);

    float3 outN;

    outN.x = (n.x - n.y);
    outN.y = (n.x + n.y) - 1.0;
    outN.z = n.z * 2.0 - 1.0;
    outN.z = outN.z * (1.0 - abs(outN.x) - abs(outN.y));

    outN = normalize(outN);
    return outN;
}

#endif