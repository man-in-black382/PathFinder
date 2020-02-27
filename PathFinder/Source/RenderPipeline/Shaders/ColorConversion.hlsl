#ifndef _ColorConvertion__
#define _ColorConvertion__

float3 LinearFromSRGB(float3 sRGB)
{
    return pow(sRGB, 2.2);
}

float3 SRGBFromLinear(float3 linearColor)
{
    return pow(linearColor, 1.0 / 2.2);
}

float Luminance(float3 color)
{
    return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
}

#endif