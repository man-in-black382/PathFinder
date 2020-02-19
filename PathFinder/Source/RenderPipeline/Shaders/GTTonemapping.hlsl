//
// http://cdn2.gran-turismo.com/data/www/pdi_publications/PracticalHDRandWCGinGTS.pdf
// https://www.desmos.com/calculator/gslcdxvipg

struct GTTonemappingParams
{
    float MaximumLuminance; // A max amount of nits a monitor can display
    float Contrast;
    float LinearSectionStart;
    float LinearSectionLength;
    float BlackTightness;
    float MinimumBrightness;
};

float GTH(float x, float e0, float e1)
{
    return saturate((x - e0) / (e1 - e0));
}

float GTW(float x, float e0, float e1)
{
    float t = GTH(x, e0, e1);
    return t * t * (3.0 - 2.0 * t);
}

float GTToneMap(float x, GTTonemappingParams params)
{
    float P = params.MaximumLuminance;
    float a = params.Contrast;
    float m = params.LinearSectionStart;
    float l = params.LinearSectionLength;
    float c = params.BlackTightness;
    float b = params.MinimumBrightness;
    float l0 = (P - m) * l / a;
    float L0 = m - m / a;
    float L1 = m + (1 - m) / a;
    float Lx = m + a * (x - m); // Linear part
    float Tx = m > 1e-5 ? m * pow(abs(x / m), c) + b : b; // Toe
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = a * P / (P - S1);
    float Sx = P - (P - S1) * exp(-C2 * (x - S0) / P); // Shoulder
    float w0x = 1.0 - GTW(x, 0.0, m);
    float w2x = x > (m + l0) ? 1 : 0;
    float w1x = 1.0 - w0x - w2x;
    
    // Divide by P to scale down to SDR [0;1] range. 
    // Will have to make it condinional if HDR output will be implemented.
    return (Tx * w0x + Lx * w1x + Sx * w2x) / P; 
}