#ifndef _ShadowNoiseEstimation__
#define _ShadowNoiseEstimation__

struct PassData
{
    uint StochasticShadowedLuminanceTextureIndex;
    uint StochasticUnshadowedLuminanceTextureIndex;
    uint OutputTextureIndex;
    float MaximumLightsLuminance;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"
#include "Packing.hlsl"
#include "Random.hlsl"
#include "Constants.hlsl"
#include "ColorConversion.hlsl"

// Number of lines to measure total variance with
static const int VarianceMeasurementLinesCount = 2;
static const int EstimationRadius = 4;
static const float EstimationRadiusInverse = 1.0 / float(EstimationRadius);
static const int GroupDimensionSize = 16;
static const int GSBufferDimensionSize = EstimationRadius * 2 + GroupDimensionSize; // Add EstimationRadius pixels around the group

// 32KB Max
groupshared uint gLuminances[GSBufferDimensionSize][GSBufferDimensionSize];

groupshared float3 gLuminancesS[GSBufferDimensionSize][GSBufferDimensionSize];
groupshared float3 gLuminancesU[GSBufferDimensionSize][GSBufferDimensionSize];

void PackLuminances(Texture2D shadowedLuminances, Texture2D unshadowedLuminances, int2 loadIndex, int2 storeIndex)
{
    float3 shadowed = shadowedLuminances[loadIndex].rgb;
    float3 unshadowed = unshadowedLuminances[loadIndex].rgb;

    // This is the maximum luminance any surface can reflect. 
    // Good fit for compression range.
    float packingRange = 100;// PassDataCB.MaximumLightsLuminance;

    // Original algorithm operates with 3-component luminances, but it's not worth performance-wise. 
    // CIE luminance gives good noise estimation but also is much more compact so we can reduce GS memory usage significantly.
    gLuminances[storeIndex.x][storeIndex.y] = PackUnorm2x16(CIELuminance(shadowed), CIELuminance(unshadowed), packingRange);

    gLuminancesS[storeIndex.x][storeIndex.y] = shadowed;
    gLuminancesU[storeIndex.x][storeIndex.y] = unshadowed;
}

void UnpackLuminances(int2 loadIndex, out float3 shadowedLuminance, out float3 unshadowedLuminance)
{
    float packingRange = 100;// PassDataCB.MaximumLightsLuminance;
    uint packed = gLuminances[loadIndex.x][loadIndex.y];
    float2 luminances = UnpackUnorm2x16(packed, packingRange);

    shadowedLuminance.r = luminances.x; unshadowedLuminance.r = luminances.y;
    shadowedLuminance.g = luminances.x; unshadowedLuminance.g = luminances.y;
    shadowedLuminance.b = luminances.x; unshadowedLuminance.b = luminances.y;

    /*shadowedLuminance = log2(shadowedLuminance);
    unshadowedLuminance = log2(unshadowedLuminance);*/

    shadowedLuminance = gLuminancesS[loadIndex.x][loadIndex.y];
    unshadowedLuminance = gLuminancesU[loadIndex.x][loadIndex.y];
}

void PopulateSharedMemory(int2 dispatchIndex, int2 threadIndex)
{
    Texture2D stochasticShadowedLuminances = Textures2D[PassDataCB.StochasticShadowedLuminanceTextureIndex];
    Texture2D stochasticUnshadowedLuminances = Textures2D[PassDataCB.StochasticUnshadowedLuminanceTextureIndex];

    dispatchIndex = clamp(dispatchIndex, 0, GlobalDataCB.PipelineRTResolution - 1);

    // Account for additional pixels (Radius) around the group
    int2 arrayIndex = threadIndex + EstimationRadius;

    PackLuminances(stochasticShadowedLuminances, stochasticUnshadowedLuminances, dispatchIndex, arrayIndex);

    // A relative coordinate for diagonal sampling, if required
    int2 diagonalOffset = 0;

    // Sample horizontally, left side
    if (threadIndex.x < EstimationRadius)
    {
        int2 loadIndex = int2(max(dispatchIndex.x - EstimationRadius, 0), dispatchIndex.y);
        int2 storeIndex = int2(arrayIndex.x - EstimationRadius, arrayIndex.y);

        diagonalOffset.x = -EstimationRadius;

        PackLuminances(stochasticShadowedLuminances, stochasticUnshadowedLuminances, loadIndex, storeIndex);
    }

    // Sample vertically, top side
    if (threadIndex.y < EstimationRadius)
    {
        int2 loadIndex = int2(dispatchIndex.x, max(dispatchIndex.y - EstimationRadius, 0));
        int2 storeIndex = int2(arrayIndex.x, arrayIndex.y - EstimationRadius);

        diagonalOffset.y = -EstimationRadius;

        PackLuminances(stochasticShadowedLuminances, stochasticUnshadowedLuminances, loadIndex, storeIndex);
    }

    // Sample horizontally, right side
    if (threadIndex.x >= (GroupDimensionSize - EstimationRadius))
    {
        int2 loadIndex = int2(min(dispatchIndex.x + EstimationRadius, GlobalDataCB.PipelineRTResolution.x - 1), dispatchIndex.y);
        int2 storeIndex = int2(arrayIndex.x + EstimationRadius, arrayIndex.y);

        diagonalOffset.x = EstimationRadius;

        PackLuminances(stochasticShadowedLuminances, stochasticUnshadowedLuminances, loadIndex, storeIndex);
    }

    // Sample vertically, bottom side
    if (threadIndex.y >= (GroupDimensionSize - EstimationRadius))
    {
        int2 loadIndex = int2(dispatchIndex.x, min(dispatchIndex.y + EstimationRadius, GlobalDataCB.PipelineRTResolution.y - 1));
        int2 storeIndex = int2(arrayIndex.x, arrayIndex.y + EstimationRadius);

        diagonalOffset.y = EstimationRadius;

        PackLuminances(stochasticShadowedLuminances, stochasticUnshadowedLuminances, loadIndex, storeIndex);
    }

    // Sample diagonally
    if (all(diagonalOffset != 0))
    {
        int2 loadIndex = clamp(int2(dispatchIndex + diagonalOffset), 0, GlobalDataCB.PipelineRTResolution - 1);
        int2 storeIndex = int2(arrayIndex + diagonalOffset);

        PackLuminances(stochasticShadowedLuminances, stochasticUnshadowedLuminances, loadIndex, storeIndex);
    }

    GroupMemoryBarrierWithGroupSync();
}

float3 GetLuminanceRatio(int2 threadIndex, int2 offset)
{
    float3 shadowedLuminance = 0; 
    float3 unshadowedLuminance = 0;

    int2 loadIndex = threadIndex + EstimationRadius + offset;

    UnpackLuminances(loadIndex, shadowedLuminance, unshadowedLuminance);

    float3 ratio = 0;

    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        ratio[i] = (unshadowedLuminance[i] < 1e-05) ? 1.0 : (shadowedLuminance[i] / unshadowedLuminance[i]);
    }

    return ratio;
}

void EstimateNoise(int2 dispatchIndex, int2 threadIndex)
{
    float angle = Random(dispatchIndex);
    float noiseLevel = 0;

    static const float Increment = Pi / float(VarianceMeasurementLinesCount);

    [unroll]
    for (int i = 0; i < VarianceMeasurementLinesCount; ++i) 
    {
        float s, c;
        sincos(angle, s, c);

        float2 direction = float2(s, c);
        float estimation = 0;

        float3 v2 = GetLuminanceRatio(threadIndex, int2(-EstimationRadius * direction));
        float3 v1 = GetLuminanceRatio(threadIndex, int2((-EstimationRadius + 1) * direction));

        float d2mag = 0.0;

        // The first two points are accounted for above
        [unroll]
        for (int r = -EstimationRadius + 2; r <= EstimationRadius; ++r) 
        {
            float3 v0 = GetLuminanceRatio(threadIndex, int2(r * direction));

            // Second derivative
            float3 d2 = v2 - v1 * 2.0 + v0;
    
            d2mag += length(d2);
    
            // Shift weights in the window
            v2 = v1; v1 = v0;
        }

        estimation = clamp(sqrt(d2mag * EstimationRadiusInverse), 0.0, 1.0);
        noiseLevel = max(estimation, noiseLevel);

        angle += Increment; 
    }

    GroupMemoryBarrierWithGroupSync();

    int2 storeIndex = threadIndex + EstimationRadius; 
    gLuminances[storeIndex.x][storeIndex.y] = asuint(noiseLevel);

    GroupMemoryBarrierWithGroupSync();
}

float DenoiseNoiseEstimation(int2 threadIndex)
{
    /*   static const int R = 1;
       float denoisedEstimation = 0.0;

       [unroll]
       for (int x = -R; x <= R; ++x)
       {
           [unroll]
           for (int y = -R; y <= R; ++y)
           {
               int2 loadIndex = threadIndex + EstimationRadius + int2(x, y);
               denoisedEstimation += asfloat(gLuminances[loadIndex.x][loadIndex.y]);
           }
       }

       denoisedEstimation *= (1.0 / Square(2.0 * float(R) + 1.0));

       return denoisedEstimation; */

    int2 loadIndex = threadIndex + EstimationRadius;
    return asfloat(gLuminances[loadIndex.x][loadIndex.y]);
}

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    RWTexture2D<float4> outputImage = RW_Float4_Textures2D[PassDataCB.OutputTextureIndex];
    
    PopulateSharedMemory(dispatchThreadID.xy, groupThreadID.xy);
    EstimateNoise(dispatchThreadID.xy, groupThreadID.xy);
    float noiseLevel = DenoiseNoiseEstimation(groupThreadID.xy);

    outputImage[dispatchThreadID.xy] = noiseLevel;
}

#endif