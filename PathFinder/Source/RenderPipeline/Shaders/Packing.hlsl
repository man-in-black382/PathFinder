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
