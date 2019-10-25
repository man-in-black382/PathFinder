float3 LinearFromSRGB(float3 sRGB)
{
    return pow(sRGB, 2.2);
}

float3 SRGBFromLinear(float3 linearColor)
{
    return pow(linearColor, 1.0 / 2.2);
}