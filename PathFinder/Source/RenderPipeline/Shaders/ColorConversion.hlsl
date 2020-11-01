#ifndef _ColorConversion__
#define _ColorConversion__

// http://www.chilliant.com/rgb2hsv.html

float3 SRGBToLinear(float3 sRGBCol)
{
    float3 linearRGBLo = sRGBCol / 12.92;
    float3 linearRGBHi = pow((sRGBCol + 0.055) / 1.055, 2.4);
    float3 linearRGB = (sRGBCol <= 0.04045) ? linearRGBLo : linearRGBHi;
    return linearRGB;
}

float3 LinearToSRGB(float3 linearCol)
{
    float3 sRGBLo = linearCol * 12.92;
    float3 sRGBHi = (pow(abs(linearCol), 1.0 / 2.4) * 1.055) - 0.055;
    float3 sRGB = (linearCol <= 0.0031308) ? sRGBLo : sRGBHi;
    return sRGB;
}

// Rec.2020 Perceptual Quantizer
float3 LinearToST2084(float3 rgb)
{
    const float m1 = 0.1593017578125;
    const float m2 = 78.84375;
    const float c1 = 0.8359375;
    const float c2 = 18.8515625;
    const float c3 = 18.6875;

    float3 ym = pow(rgb, m1);
    return pow((c1 + c2 * ym) / (1 + c3 * ym), m2);
}

// Color gamut conversion: Rec.709 -> Rec.2020
float3 Rec709ToRec2020(float3 color)
{
    static const float3x3 conversion =
    {
        0.627402, 0.329292, 0.043306,
        0.069095, 0.919544, 0.011360,
        0.016394, 0.088028, 0.895578
    };
    return mul(conversion, color);
}

// Color gamut conversion: Rec.2020 -> Rec.709
float3 Rec2020ToRec709(float3 color)
{
    static const float3x3 conversion =
    {
        1.660496, -0.587656, -0.072840,
        -0.124547, 1.132895, -0.008348,
        -0.018154, -0.100597, 1.118751
    };
    return mul(conversion, color);
}

float CIELuminance(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

float3 RGBtoHCV(float3 RGB)
{
    const float Epsilon = 1e-10;
    // Based on work by Sam Hocevar and Emil Persson
    float4 P = (RGB.g < RGB.b) ? float4(RGB.bg, -1.0, 2.0 / 3.0) : float4(RGB.gb, 0.0, -1.0 / 3.0);
    float4 Q = (RGB.r < P.x) ? float4(P.xyw, RGB.r) : float4(RGB.r, P.yzx);
    float C = Q.x - min(Q.w, Q.y);
    float H = abs((Q.w - Q.y) / (6 * C + Epsilon) + Q.z);
    return float3(H, C, Q.x);
}

float3 HUEtoRGB(float H)
{
    float R = abs(H * 6 - 3) - 1;
    float G = 2 - abs(H * 6 - 2);
    float B = 2 - abs(H * 6 - 4);
    return saturate(float3(R, G, B));
}

float3 HSVtoRGB(float3 HSV)
{
    float3 RGB = HUEtoRGB(HSV.x);
    return ((RGB - 1) * HSV.y + 1) * HSV.z;
}

float3 RGBtoHSV(float3 RGB)
{
    const float Epsilon = 1e-10;
    float3 HCV = RGBtoHCV(RGB);
    float S = HCV.y / (HCV.z + Epsilon);
    return float3(HCV.x, S, HCV.z);
}

// This function take a rgb color (best is to provide color in sRGB space)
// and return a YCoCg color in [0..1] space for 8bit (An offset is apply in the function)
// Ref: http://www.nvidia.com/object/real-time-ycocg-dxt-compression.html
//
float3 RGBToYCoCg(float3 rgb)
{
    float3 YCoCg;
    YCoCg.x = dot(rgb, float3(0.25, 0.5, 0.25));
    YCoCg.y = dot(rgb, float3(0.5, 0.0, -0.5));
    YCoCg.z = dot(rgb, float3(-0.25, 0.5, -0.25));

    return YCoCg;
}

float3 YCoCgToRGB(float3 YCoCg)
{
    float Y = YCoCg.x;
    float Co = YCoCg.y;
    float Cg = YCoCg.z;

    float3 rgb;
    rgb.r = Y + Co - Cg;
    rgb.g = Y + Cg;
    rgb.b = Y - Co - Cg;

    return rgb;
}

#endif