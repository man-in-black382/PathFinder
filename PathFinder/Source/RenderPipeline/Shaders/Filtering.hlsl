#ifndef _Filtering__
#define _Filtering__

// Numerically stable manual bilinear filtering
// https://community.khronos.org/t/manual-bilinear-filter/58504/8
//
struct Bilinear
{
    float2 TopLeftUV;
    float2 TexelSize;
    float2 Weights; 
};

struct GatheredRGBA
{
    float4 Red;
    float4 Green;
    float4 Blue;
    float4 Alpha;
};

Bilinear GetBilinearFilter(float2 uv, float2 texelSize, float2 textureSize)
{
    float2 halfTexelSize = texelSize * 0.5;

    // Move to texel corner
    float2 corner = uv - halfTexelSize;
    float2 f = frac(corner * textureSize);

    Bilinear filter;
    filter.TexelSize = texelSize;
    filter.Weights = 1.0 - frac(corner * textureSize);
    filter.TopLeftUV = corner + (0.5 - f) * texelSize; // Move to texel center
    return filter;
}

float4 GetBilinearCustomWeights(Bilinear f, float4 customWeights)
{
    float4 weights;

    // Expand lerp-s into separate weights
    weights.x = f.Weights.x * f.Weights.y; 
    weights.y = (1.0 - f.Weights.x) * f.Weights.y; 
    weights.w = f.Weights.x * (1.0 - f.Weights.y); 
    weights.z = (1.0 - f.Weights.x) * (1.0 - f.Weights.y); 

    // Apply custom weights
    weights *= customWeights;

    // Renormalize
    float sum = max(dot(weights, 1.0), 1e-04); // Prevent div by 0
    weights /= sum;

    return weights;
}

float ApplyBilinearCustomWeights(float4 v, float4 weights)
{
    // Weight
    v *= weights;
    // Sum
    return dot(v, 1.0);
}

float4 GatherRedManually(Texture2D tex, Bilinear filter, float mipLevel, sampler s)
{
    // Get 4 neighbor texel values
    float tl = tex.SampleLevel(s, filter.TopLeftUV, mipLevel).r;
    float tr = tex.SampleLevel(s, filter.TopLeftUV + float2(filter.TexelSize.x, 0.0), mipLevel).r;
    float bl = tex.SampleLevel(s, filter.TopLeftUV + float2(0.0, filter.TexelSize.y), mipLevel).r;
    float br = tex.SampleLevel(s, filter.TopLeftUV + float2(filter.TexelSize.x, filter.TexelSize.y), mipLevel).r;

    return float4(tl.r, tr.r, br.r, bl.r);
}

GatheredRGBA GatherRGBAManually(Texture2D tex, Bilinear filter, float mipLevel, sampler s)
{
    // Get 4 neighbor texel values
    float4 tl = tex.SampleLevel(s, filter.TopLeftUV, mipLevel);
    float4 tr = tex.SampleLevel(s, filter.TopLeftUV + float2(filter.TexelSize.x, 0.0), mipLevel);
    float4 bl = tex.SampleLevel(s, filter.TopLeftUV + float2(0.0, filter.TexelSize.y), mipLevel);
    float4 br = tex.SampleLevel(s, filter.TopLeftUV + float2(filter.TexelSize.x, filter.TexelSize.y), mipLevel);

    GatheredRGBA result;
    result.Red = float4(tl.r, tr.r, br.r, bl.r);
    result.Green = float4(tl.g, tr.g, br.g, bl.g);
    result.Blue = float4(tl.b, tr.b, br.b, bl.b);
    result.Alpha = float4(tl.a, tr.a, br.a, bl.a);

    return result;
}

#endif