#ifndef _SpecularDenoiser__
#define _SpecularDenoiser__

#include "GBuffer.hlsl"

struct PassData
{
    GBufferTextureIndices GBufferIndices;
    uint AccumulatedFramesCountTexIdx;
    uint ShadowedShadingHistoryTexIdx;
    uint UnshadowedShadingHistoryTexIdx;
    uint CurrentShadowedShadingTexIdx;
    uint CurrentUnshadowedShadingTexIdx;
    uint ShadowedShadingDenoiseTargetTexIdx;
    uint UnshadowedShadingDenoiseTargetTexIdx;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"
#include "DenoiserCommon.hlsl"
#include "Matrix.hlsl"
#include "Random.hlsl"

static const int GroupDimensionSize = 16;
static const int DenoiseSampleCount = 8; 

// https://developer.nvidia.com/gtc/2020/video/s22699

float MaxAllowedAccumulatedFrames(float roughness, float NdotV, float parallax)
{
    // NdotV represents sensitivity to parallax - more sensitive under glancing angles
    // (accumulation speed becomes faster)

    // Controls aggressiveness of history rejection depending on viewing angle
    // Smaller values - less accumulation under glancing values
    static const float SpecularAccumulationCurve = 0.2;
     
    // Controls sensitivity to parallax in general
    // Smaller values - more aggressive accumulation
    static const float SpecularAccumulationBasePower = 0.25;

    float acos01sq = saturate(1.0 - NdotV); // ~ "normalized acos" ^ 2
    float a = pow(acos01sq, SpecularAccumulationCurve);
    float b = 1.001 + roughness * roughness;
    float angularSensitivity = (b + a) / (b - a);
    float power = SpecularAccumulationBasePower * (1.0 + parallax * angularSensitivity);

    return MaxAccumulatedFrames * pow(roughness, power);
}

float3x3 SamplingBasis(float3 Xview, float3 Nview, float roughness, float radiusScale, float normAccumFrameCount)
{
    float3 V = -normalize(Xview);
    float3 D = GGXDominantDirection(Nview, V, roughness);
    float3 R = reflect(-D, Nview);

    float3 T = normalize(cross(Nview, R));
    float3 B = cross(R, T);

    T *= radiusScale;
    B *= radiusScale;

    // Normal Z in view space is a cosine between surface normal and view direction in camera view space. 
    float angleNormalized = 1.0 - abs(Nview.z);
    float skewFactor = lerp(1.0, roughness, angleNormalized);

    T *= lerp(1.0, skewFactor, normAccumFrameCount);

    return Matrix3x3ColumnMajor(T, B, R);
}

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    uint2 pixelIndex = dispatchThreadID.xy;
    float2 uv = (float2(pixelIndex) + 0.5) / (GlobalDataCB.PipelineRTResolution);

    GBufferTexturePack gBufferTextures;
    gBufferTextures.NormalRoughness = Textures2D[PassDataCB.GBufferIndices.NormalRoughnessTexIdx];
    gBufferTextures.Motion = UInt4_Textures2D[PassDataCB.GBufferIndices.MotionTexIdx];
    gBufferTextures.DepthStencil = Textures2D[PassDataCB.GBufferIndices.DepthStencilTexIdx];
    gBufferTextures.ViewDepth = Textures2D[PassDataCB.GBufferIndices.ViewDepthTexIdx];

    Texture2D accumulatedFramesCountTexture = Textures2D[PassDataCB.AccumulatedFramesCountTexIdx];
    Texture2D currentShadowedShadingTexture = Textures2D[PassDataCB.CurrentShadowedShadingTexIdx];
    Texture2D currentUnshadowedShadingTexture = Textures2D[PassDataCB.CurrentUnshadowedShadingTexIdx];
    Texture2D shadowedShadingHistoryTexture = Textures2D[PassDataCB.ShadowedShadingHistoryTexIdx];
    Texture2D unshadowedShadingHistoryTexture = Textures2D[PassDataCB.UnshadowedShadingHistoryTexIdx];

    RWTexture2D<float4> shadowedShadingDenoiseTargetTexture = RW_Float4_Textures2D[PassDataCB.ShadowedShadingDenoiseTargetTexIdx];
    RWTexture2D<float4> unshadowedShadingDenoiseTargetTexture = RW_Float4_Textures2D[PassDataCB.UnshadowedShadingDenoiseTargetTexIdx];

    float accumFramesCount = accumulatedFramesCountTexture.Load(uint3(pixelIndex, 0)).r;
    float depth = gBufferTextures.DepthStencil.Load(uint3(pixelIndex, 0)).r;

    if (depth >= 1.0)
    {
        shadowedShadingDenoiseTargetTexture[pixelIndex].rgb = 0.0;
        unshadowedShadingDenoiseTargetTexture[pixelIndex].rgb = 0.0;
        return;
    }

    float3 motion = 0.0;// LoadGBufferMotion(gBufferTextures.Motion, pixelIndex);
    float frameTime = 1.0 / 60.0; // TODO: pass from CPU

    float3 worldPosition, viewPosition; 
    ReconstructPositions(depth, uv, FrameDataCB.CurrentFrameCamera, viewPosition, worldPosition);

    float roughness;
    float3 worldNormal;

    LoadGBufferNormalAndRoughness(gBufferTextures.NormalRoughness, pixelIndex, worldNormal, roughness);

    // Compute algorithm inputs
    float3 viewNormal = mul(ReduceTo3x3(FrameDataCB.CurrentFrameCamera.View), worldNormal);
    float3 prevWorldPosition = worldPosition - motion;
    float3 cameraDelta = FrameDataCB.CurrentFrameCamera.Position.xyz - FrameDataCB.PreviousFrameCamera.Position.xyz;
    float3 movemenDelta = worldPosition - (prevWorldPosition - cameraDelta);
    float distanceToPoint = distance(FrameDataCB.PreviousFrameCamera.Position.xyz, prevWorldPosition);
    float3 viewVector = normalize(FrameDataCB.CurrentFrameCamera.Position.xyz - worldPosition);
    float NdotV = dot(worldNormal, viewVector);

    // ~sine of angle between old and new view vector in world space
    // Measure of relative surface-camera motion.
    float parallax = length(movemenDelta) / (distanceToPoint * frameTime);

    // Decrease maximum frame number on surface motion which will effectively decrease history weight,
    // which is mandatory to combat ghosting
    float maxAllowedAccumFrames = MaxAllowedAccumulatedFrames(roughness, NdotV, parallax);
    //accumFramesCount = min(accumFramesCount, maxAllowedAccumFrames);
    float accumFramesCountNorm = accumFramesCount / maxAllowedAccumFrames;
    float accumulationSpeed = 1.0 / (1.0 + accumFramesCount);

    // Define blur radius range
    const float MinBlurRadius = 0.01;
    const float MaxBlurRadius = 0.1;
    
    // Decrease blur radius as more frames are accumulated
    float blurRadius = lerp(MinBlurRadius, MaxBlurRadius, 1.0 - accumFramesCountNorm);

    // Prepare sampling basis which is a scale and rotation matrix
    float3x3 samplingBasis = SamplingBasis(viewPosition, viewNormal, roughness, blurRadius, accumFramesCountNorm);

    // Get a random rotation to be applied for each sample
    float vogelDiskRotation = Random(pixelIndex.xy) * TwoPi;

    // Get center sample
    float3 denoisedShadowed = currentShadowedShadingTexture[pixelIndex].rgb;
    float3 denoisedUnshadowed = currentUnshadowedShadingTexture[pixelIndex].rgb;
    
    float totalWeight = 1.0;

    [unroll]
    for (int i = 0; i < DenoiseSampleCount; ++i)
    {
        // Generate sample in 2D
        float2 vdSample = VogelDiskSample(i, DenoiseSampleCount, vogelDiskRotation);

        // Make sample 3D and z-alighned
        float3 vd3DSample = float3(vdSample, 0.0);

        // Transform sample into its basis.
        // Then position it around surface point in camera view space.
        float3 viewSpaceSample = mul(samplingBasis, vd3DSample) + viewPosition;

        // Project and obtain sample's UV
        float3 projectedSample = ProjectPoint(viewSpaceSample, FrameDataCB.CurrentFrameCamera);
        float2 sampleUV = NDCToUV(projectedSample);

        // Get neighbor properties
        float4 neighborNormalRoughness = gBufferTextures.NormalRoughness.SampleLevel(LinearClampSampler, sampleUV, 0);

        float3 neighborNormal = ExpandGBufferNormal(neighborNormalRoughness.xyz);
        float neighborRoughness = neighborNormalRoughness.w;

        // Compute weights
        float normalWeight = NormalWeight(worldNormal, neighborNormal, roughness, accumFramesCount);
        float geometryWeight = GeometryWeight(viewPosition, viewNormal, viewSpaceSample, viewPosition.z);
        float roughnessWeight = RoughnessWeight(roughness, neighborRoughness);

        float sampleWeight = normalWeight * geometryWeight * roughnessWeight;

        // Sample neighbor value and weight accordingly
        denoisedShadowed += currentShadowedShadingTexture.SampleLevel(LinearClampSampler, sampleUV, 0).rgb * sampleWeight;
        denoisedUnshadowed += currentUnshadowedShadingTexture.SampleLevel(LinearClampSampler, sampleUV, 0).rgb * sampleWeight;

        totalWeight += sampleWeight;
    }

    denoisedShadowed /= totalWeight;
    denoisedUnshadowed /= totalWeight;

    // Combine with history
    denoisedShadowed = lerp(shadowedShadingHistoryTexture[pixelIndex].rgb, denoisedShadowed, accumulationSpeed);
    denoisedUnshadowed = lerp(unshadowedShadingHistoryTexture[pixelIndex].rgb, denoisedUnshadowed, accumulationSpeed);
    
    // Output final denoised value
    shadowedShadingDenoiseTargetTexture[pixelIndex].rgb = denoisedShadowed;
    unshadowedShadingDenoiseTargetTexture[pixelIndex].rgb = denoisedUnshadowed;
}

#endif