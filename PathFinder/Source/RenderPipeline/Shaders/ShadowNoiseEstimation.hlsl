#ifndef _ShadowNoiseEstimation__
#define _ShadowNoiseEstimation__

struct PassData
{
    uint StochasticShadowedLuminanceTextureIndex;
    uint StochasticUnshadowedLuminanceTextureIndex;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const uint R = 10;

///* Sample the signal used for the noise estimation at offset from ssC */
//float3 tap(uint2 pixelIndex, uint2 offset, Texture2D source0, Texture2D source1)
//{
////#if 0
////
////    // stochastic image only
////    // source0 - baseline
////    return texelFetch(source0, ssC + offset, 0).rgb - texelFetch(baseline, ssC + offset, 0).rgb;
////#else
//    // Conditional shadow; gives slightly noiser transition to penumbra but avoids noise in the umbra
//    float3 n = source0.Load(uint3(pixelIndex + offset, 0)).rgb;
//    float3 d = source1.Load(uint3(pixelIndex + offset, 0)).rgb;
//
//    float3 result;
//    for (int i = 0; i < 3; ++i) 
//    {
//        result[i] = (d[i] < 0.000001) ? 1.0 : (n[i] / d[i]);
//    }
//    return result;
////#endif
//}
//
/////////////////////////////////////////////////////////////////////////////////
//// Estimate desired radius from the second derivative of the signal itself in source0 relative to baseline,
//// which is the noisier image because it contains shadows
//float estimateNoise(vec2 axis)
//{
//    const int NOISE_ESTIMATION_RADIUS = min(10, R);
//    vec3 v2 = tap(ivec2(-NOISE_ESTIMATION_RADIUS * axis));
//    vec3 v1 = tap(ivec2((1 - NOISE_ESTIMATION_RADIUS) * axis));
//
//    float d2mag = 0.0;
//    // The first two points are accounted for above
//    for (int r = -NOISE_ESTIMATION_RADIUS + 2; r <= NOISE_ESTIMATION_RADIUS; ++r) 
//    {
//        float3 v0 = tap(ivec2(axis * r));
//
//        // Second derivative
//        float3 d2 = v2 - v1 * 2.0 + v0;
//
//        d2mag += length(d2);
//
//        // Shift weights in the window
//        v2 = v1; v1 = v0;
//    }
//
//    // Scaled value by 1.5 *before* clamping to visualize going out of range
//    // It is clamped again when applied.
//    return clamp(sqrt(d2mag * (1.0 / float(R))) * (1.0 / 1.5), 0.0, 1.0);
//}
//
//void main()
//{
//
//    float angle = hash(gl_FragCoord.x + gl_FragCoord.y * 1920.0);
//
//    result = 0;
//    const int N = 4;
//    for (float t = 0; t < N; ++t, angle += pi / float(N)) {
//        float c = cos(angle), s = sin(angle);
//        result = max(estimateNoise(vec2(c, s)), result);
//    }
//
//}


[numthreads(32, 32, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    //Texture2D inputImage = Textures2D[PassDataCB.InputTextureIndex];
    //RWTexture2D<float4> outputImage = RW_Float4_Textures2D[PassDataCB.OutputTextureIndex];
    //
    //uint3 loadCoords = uint3(dispatchThreadID.xy, 0);
    //float3 color = inputImage.Load(loadCoords).rgb;

    //GTTonemappingParams params = PassDataCB.TonemappingParams;
    //// Luminance was exposed using Saturation Based Sensitivity method 
    //// hence the 1.0 for maximum luminance
    //params.MaximumLuminance = 1.0; 

    //float3 tonemappedColor = float3(
    //    GTToneMap(color.r, params),
    //    GTToneMap(color.g, params),
    //    GTToneMap(color.b, params)
    //);

    //outputImage[dispatchThreadID.xy] = float4(SRGBFromLinear(tonemappedColor), 1.0);

    //uint2 resolution = (GlobalDataCB.PipelineRTResolution - 1);

    //float3 average = 0.xxx;
    //uint2 pixelIndex = pin.UV * resolution;

    //for (uint i = 0; i < 6; ++i)
    //{
    //    for (uint j = 0; j < 6; ++j)
    //    {
    //        uint2 loadIndex = pixelIndex + uint2(i, j);
    //        loadIndex = clamp(loadIndex, 0, resolution);

    //        float3 color = source.Load(uint3(loadIndex, 0)).rgb;
    //        average += color;
    //    }
    //}

    //average /= 36;

    //return float4(average, 1.0);

}

#endif