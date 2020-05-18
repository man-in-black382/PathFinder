#ifndef _ShadowNoiseEstimation__
#define _ShadowNoiseEstimation__

struct PassData
{
    uint NoiseEstimationTexIdx;
    uint AnalyticLuminanceTexIdx;
    uint StochasticShadowedLuminanceTexIdx;
    uint StochasticUnshadowedLuminanceTexIdx;
    // 16 byte boundary
    uint GBufferTexIdx;
    uint DepthTexIdx;
    uint IntermediateOutput0TexIdx;
    uint IntermediateOutput1TexIdx;
    // 16 byte boundary
    float2 ImageSize;
    uint FinalOutputTexIdx;
    float MaximumLightsLuminance;
    // 16 byte boundary
    uint IsHorizontal;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"
#include "Packing.hlsl"
#include "Random.hlsl"
#include "Constants.hlsl"
#include "ColorConversion.hlsl"
#include "GBuffer.hlsl"
#include "Gaussian.hlsl"
#include "Matrix.hlsl"

static const int MaxDenoiseRadius = 10;
static const int GroupDimensionSize = 256;
static const int GSBufferDimensionSize = MaxDenoiseRadius * 2 + GroupDimensionSize;

static const float DepthWeightContribution = 1.0;
static const float PlaneWeightContribution = 1.0;
static const float NormalWeightContribution = 1.5;
static const float AnalyticLuminanceWeightContribution = 0.09;

groupshared float3 gLuminancesS[GSBufferDimensionSize]; // Stochastic luminances
groupshared float3 gLuminancesU[GSBufferDimensionSize]; // Stochastic luminances
groupshared uint3 gTapKeyData[GSBufferDimensionSize]; // Packed tap key data

struct TapKey 
{
    float CSDepth; // Camera Space
    float3 CSPosition; // Camera Space
    float3 CSNormal;
    float Analytic;
};

void PackLuminances(RWTexture2D<float4> shadowedLuminances, RWTexture2D<float4> unshadowedLuminances, int2 loadIndex, int storeIndex)
{
    float3 shadowed = shadowedLuminances[loadIndex].rgb;
    float3 unshadowed = unshadowedLuminances[loadIndex].rgb;

    if (any(shadowed < 0.0)) shadowed = float3(1, 0, 0);
    if (any(unshadowed < 0.0)) unshadowed = float3(1, 0, 0);

    gLuminancesS[storeIndex] = shadowed;
    gLuminancesU[storeIndex] = unshadowed;

    //// This is the maximum luminance any surface can reflect. 
    //// Good fit for compression range.
    //float packingRange = PassDataCB.MaximumLightsLuminance;

    //gLuminances[storeIndex] = uint3(
    //    PackUnorm2x16(shadowed.r, unshadowed.r, packingRange),
    //    PackUnorm2x16(shadowed.g, unshadowed.g, packingRange),
    //    PackUnorm2x16(shadowed.b, unshadowed.b, packingRange));
}

void UnpackLuminances(int threadIndex, out float3 shadowedLuminance, out float3 unshadowedLuminance)
{
    int loadIndex = threadIndex + MaxDenoiseRadius;
   /* float packingRange = PassDataCB.MaximumLightsLuminance;
    uint3 packed = gLuminances[loadIndex];
    float2 r0r1 = UnpackUnorm2x16(packed.x, packingRange);
    float2 g0g1 = UnpackUnorm2x16(packed.y, packingRange);
    float2 b0b1 = UnpackUnorm2x16(packed.z, packingRange);

    shadowedLuminance.r = r0r1[0]; unshadowedLuminance.r = r0r1[1];
    shadowedLuminance.g = g0g1[0]; unshadowedLuminance.g = g0g1[1];
    shadowedLuminance.b = b0b1[0]; unshadowedLuminance.b = b0b1[1];*/

    shadowedLuminance = gLuminancesS[loadIndex];
    unshadowedLuminance = gLuminancesU[loadIndex];
}

void PackTapKey(Texture2D<uint4> gBufferTexture, Texture2D analyticLuminances, Texture2D depthTexture, int2 loadIndex, int storeIndex)
{
   /* GBufferEncoded encodedGBuffer;
    encodedGBuffer.Encoded = gBufferTexture.Load(uint3(loadIndex, 0));

    uint gBufferType = DecodeGBufferType(encodedGBuffer);

    [branch] switch (gBufferType)
    {
    case GBufferTypeStandard:
    {
        float3 normal = mul(ReduceTo3x3(FrameDataCB.CurrentFrameCamera.View), DecodeGBufferStandardNormal(encodedGBuffer));
        float depth = depthTexture.Load(uint3(loadIndex, 0)).r;
        float3 analyticLuminance = analyticLuminances.Load(uint3(loadIndex, 0)).rgb;
        float luminanceMean = (analyticLuminance.r + analyticLuminance.g + analyticLuminance.b) * 0.3333333;
        uint normalRepacked = EncodeNormalSignedOct(normal);

        gTapKeyData[storeIndex] = uint3(normalRepacked, asuint(depth), asuint(luminanceMean));
        break;
    }

    case GBufferTypeEmissive:
    {
        gTapKeyData[storeIndex] = 0;
        break;
    }
    }*/
}

TapKey UnpackTapKey(float2 uv, int threadIndex)
{
    int loadIndex = threadIndex + MaxDenoiseRadius;
    uint3 packed = gTapKeyData[loadIndex];
    float depth = asfloat(packed.y);

    TapKey key;
    key.CSDepth = LinearizeDepth(depth, FrameDataCB.CurrentFrameCamera);
    key.CSPosition = ReconstructViewSpacePosition(depth, uv, FrameDataCB.CurrentFrameCamera);
    key.CSNormal = DecodeNormalSignedOct(packed.x);
    key.Analytic = asfloat(packed.z);

    return key;
}

void PopulateSharedMemory(int2 dispatchIndex, int threadIndex, bool isHorizontal)
{
    Texture2D analyticLuminances = Textures2D[PassDataCB.AnalyticLuminanceTexIdx];
    RWTexture2D<float4> stochasticShadowedLuminances = RW_Float4_Textures2D[PassDataCB.StochasticShadowedLuminanceTexIdx];
    RWTexture2D<float4> stochasticUnshadowedLuminances = RW_Float4_Textures2D[PassDataCB.StochasticUnshadowedLuminanceTexIdx];
    Texture2D<uint4> gBuffer = UInt4_Textures2D[PassDataCB.GBufferTexIdx];
    Texture2D depthTexture = Textures2D[PassDataCB.DepthTexIdx];

    dispatchIndex = clamp(dispatchIndex, 0, GlobalDataCB.PipelineRTResolution - 1);

    // Account for additional pixels (Radius) around the group
    int storeCoord = threadIndex + MaxDenoiseRadius;

    PackLuminances(stochasticShadowedLuminances, stochasticUnshadowedLuminances, dispatchIndex, storeCoord);
    PackTapKey(gBuffer, analyticLuminances, depthTexture, dispatchIndex, storeCoord);

    if (threadIndex < MaxDenoiseRadius) 
    {
        int2 loadCoord = isHorizontal ? 
            int2(max(dispatchIndex.x - MaxDenoiseRadius, 0), dispatchIndex.y) :
            int2(dispatchIndex.x, max(dispatchIndex.y - MaxDenoiseRadius, 0));

        int smCoord = storeCoord - MaxDenoiseRadius;

        PackLuminances(stochasticShadowedLuminances, stochasticUnshadowedLuminances, loadCoord, smCoord);
        PackTapKey(gBuffer, analyticLuminances, depthTexture, loadCoord, smCoord);
    }

    if (threadIndex >= (GroupDimensionSize - MaxDenoiseRadius))
    {
        int2 loadCoord = isHorizontal ?
            int2(min(dispatchIndex.x + MaxDenoiseRadius, GlobalDataCB.PipelineRTResolution.x - 1), dispatchIndex.y) :
            int2(dispatchIndex.x, min(dispatchIndex.y + MaxDenoiseRadius, GlobalDataCB.PipelineRTResolution.y - 1));

        int smCoord = storeCoord + MaxDenoiseRadius;

        PackLuminances(stochasticShadowedLuminances, stochasticUnshadowedLuminances, loadCoord, smCoord);
        PackTapKey(gBuffer, analyticLuminances, depthTexture, loadCoord, smCoord);
    }

    GroupMemoryBarrierWithGroupSync();
}

float ComputeBilateralWeight(TapKey centerTapKey, TapKey neighbourTapKey)
{
    float depthWeight = 1.0;
    float normalWeight = 1.0;
    float planeWeight = 1.0;
    float analyticWeight = 1.0;

    if (DepthWeightContribution > 0.0) 
    {
        depthWeight = max(0.0, 1.0 - abs(neighbourTapKey.CSDepth - centerTapKey.CSDepth) * DepthWeightContribution);
    }

    if (NormalWeightContribution > 0.0)
    {
        float normalCloseness = dot(neighbourTapKey.CSNormal, centerTapKey.CSNormal);
        normalCloseness = normalCloseness * normalCloseness;
        normalCloseness = normalCloseness * normalCloseness;

        float normalError = (1.0 - normalCloseness);
        normalWeight = max((1.0 - normalError * NormalWeightContribution), 0.00);
    }

    if (PlaneWeightContribution > 0.0)
    {
        float lowDistanceThreshold2 = 0.001;

        // Change in position in camera space
        float3 dq = centerTapKey.CSPosition - neighbourTapKey.CSPosition;

        // How far away is this point from the original sample
        // in camera space? (Max value is unbounded)
        float distance2 = dot(dq, dq);

        // How far off the expected plane (on the perpendicular) is this point?  Max value is unbounded.
        float planeError = max(abs(dot(dq, neighbourTapKey.CSNormal)), abs(dot(dq, centerTapKey.CSNormal)));

        planeWeight = (distance2 < lowDistanceThreshold2) ? 1.0 :
            pow(max(0.0, 1.0 - 2.0 * PlaneWeightContribution * planeError / sqrt(distance2)), 2.0);
    }

    if (AnalyticLuminanceWeightContribution > 0.0) 
    {
        float aDiff = abs(neighbourTapKey.Analytic - centerTapKey.Analytic);
        analyticWeight = max(0.0, 1.0 - (aDiff * AnalyticLuminanceWeightContribution));
    }

    return depthWeight * normalWeight * planeWeight * analyticWeight;
}

[numthreads(GroupDimensionSize, 1, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    // UV intentionally does not depend on swapped dispatch id
    float2 uv = (float2(dispatchThreadID.xy) + 0.5) / (PassDataCB.ImageSize - 1);
    // To reuse the same shader with horizontal group both for vertical and horizontal blur we swap dispatch id's X and Y
    int2 texelIndex = PassDataCB.IsHorizontal ? dispatchThreadID.xy : dispatchThreadID.yx;

    Texture2D noiseEstimation = Textures2D[PassDataCB.NoiseEstimationTexIdx];
    Texture2D analyticLuminances = Textures2D[PassDataCB.AnalyticLuminanceTexIdx];

    float3 analyticLuminance = analyticLuminances[texelIndex].rgb;
    float estimatedNoise = noiseEstimation[texelIndex].r;
    
    PopulateSharedMemory(texelIndex, groupThreadID.x, PassDataCB.IsHorizontal);

    float gaussianRadius = estimatedNoise* float(MaxDenoiseRadius);

    float3 denoisedShadowed = 0.0;
    float3 denoisedUnshadowed = 0.0;

    if (gaussianRadius >= 1.0) 
    {
        float3 sum0 = 0.0;
        float3 sum1 = 0.0;
        float totalWeight = 0.0;

        IncrementalGaussian gaussian = GaussianApproximation(gaussianRadius * 4);
        TapKey centralTapKey = UnpackTapKey(uv, groupThreadID.x);

        // Process central pixel
        float3 stochasticUnshadowedLuminance, stochasticShadowedLuminance;
        UnpackLuminances(groupThreadID.x, stochasticShadowedLuminance, stochasticUnshadowedLuminance);

        float centralWeight = GetIncrementalGaussianWeight(gaussian);
        sum0 += stochasticShadowedLuminance * centralWeight;
        sum1 += stochasticUnshadowedLuminance * centralWeight;
        totalWeight += centralWeight;

        for (int r = 1; r <= gaussianRadius; ++r)
        {
            int leftIndex = groupThreadID.x - r;
            int rightIndex = groupThreadID.x + r;

            TapKey leftNeighborTapKey = UnpackTapKey(uv, leftIndex);
            TapKey rightNeighborTapKey = UnpackTapKey(uv, rightIndex);

            float gaussianWeight = IncrementGaussian(gaussian);
            float leftWeight = gaussianWeight * ComputeBilateralWeight(centralTapKey, leftNeighborTapKey);
            float rightWeight = gaussianWeight * ComputeBilateralWeight(centralTapKey, rightNeighborTapKey);

            UnpackLuminances(leftIndex, stochasticShadowedLuminance, stochasticUnshadowedLuminance);
            sum0 += stochasticShadowedLuminance * leftWeight;
            sum1 += stochasticUnshadowedLuminance * leftWeight;

            UnpackLuminances(rightIndex, stochasticShadowedLuminance, stochasticUnshadowedLuminance);
            sum0 += stochasticShadowedLuminance * rightWeight;
            sum1 += stochasticUnshadowedLuminance * rightWeight;

            totalWeight += leftWeight + rightWeight;
        }

        denoisedShadowed = sum0 / totalWeight;
        denoisedUnshadowed = sum1 / totalWeight;
    }
    else
    {
        UnpackLuminances(groupThreadID.x, denoisedShadowed, denoisedUnshadowed);
    }

    if (PassDataCB.IsHorizontal)
    {
        // Horizontal pass outputs to intermediate buffers
        RWTexture2D<float4> intermediateOutput0 = RW_Float4_Textures2D[PassDataCB.IntermediateOutput0TexIdx];
        RWTexture2D<float4> intermediateOutput1 = RW_Float4_Textures2D[PassDataCB.IntermediateOutput1TexIdx];

        intermediateOutput0[texelIndex] = float4(denoisedShadowed, 1.0);
        intermediateOutput1[texelIndex] = float4(denoisedUnshadowed, 1.0); 
    }
    else
    {
        // Vertical pass composes the final image
        RWTexture2D<float4> outputImage = RW_Float4_Textures2D[PassDataCB.FinalOutputTexIdx];

        float3 ratio = 0.0;

        // Threshold degenerate values
        [unroll] for (int i = 0; i < 3; ++i)
        {
            ratio[i] = denoisedUnshadowed[i] < 0.00001 ? 1.0 : (denoisedShadowed[i] / denoisedUnshadowed[i]);
        } 

        float3 finalLuminance = analyticLuminance * ratio;
        outputImage[texelIndex] = float4(finalLuminance, 1.0);
    }
}

#endif