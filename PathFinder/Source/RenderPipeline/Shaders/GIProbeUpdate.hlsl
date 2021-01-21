#ifndef _GIProbeUpdate__
#define _GIProbeUpdate__

#include "GIProbeHelpers.hlsl"
#include "Packing.hlsl"

struct PassData
{
    IrradianceField ProbeField;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

// Following constants *must* match values in IrradianceField
static const uint IrradianceProbeSize = 8;
static const uint IrradianceProbeTexelCount = IrradianceProbeSize * IrradianceProbeSize;
static const uint DepthProbeSize = 16;
static const uint DepthProbeTexelCount = DepthProbeSize * DepthProbeSize;
static const uint RaysPerProbe = 64;
static const float EnergyConservation = 0.95;
static const float DepthSharpness = 1.0;

groupshared float4 RayHitInfo[RaysPerProbe];

// We use largest probe dimension to accommodate both probe types
[numthreads(DepthProbeTexelCount, 1, 1)]
void CSMain(uint3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
    // We work with depth texel counts because it's the larger probe of the two 
    uint probeIndex = dtID.x / DepthProbeTexelCount;
    uint probeLocal1DTexelIndex = gtID.x % DepthProbeTexelCount;
    uint2 probeLocal2DTexelIndex = Index2DFrom1D(probeLocal1DTexelIndex, DepthProbeSize);
    uint3 probe3DIndex = Probe3DIndexFrom1D(probeIndex, PassDataCB.ProbeField);
    float3 probePosition = ProbePositionFrom3DIndex(probe3DIndex, PassDataCB.ProbeField);

    // Read ray hit info into shared memory.
    // It is important to have enough threads in the group 
    // to read out all of the ray hit info values (RaysPerProbe <= LargestProbeTypeTexelCount)
    // otherwise we won't read all the values and algorithm would break.
    uint rayIndex = probeLocal1DTexelIndex;

    // Read only if there's anything to read. If texel index is larger than ray count then we've read all the ray info already.
    if (rayIndex < RaysPerProbe)
    {
        uint2 rayTexelIdx = RayHitTexelIndex(rayIndex, probeIndex, PassDataCB.ProbeField);
        RayHitInfo[rayIndex] = Textures2D[PassDataCB.ProbeField.RayHitInfoTextureIdx][rayTexelIdx];
    }

    GroupMemoryBarrierWithGroupSync();

    float4 irradianceResult = 0.0;
    float4 depthResult = 0.0;

    const float Epsilon = 0.0001;

    // For each ray
    for (uint rayIdx = 0; rayIdx < RaysPerProbe; ++rayIdx)
    {
        // We restore ray direction here knowing current frame ray generation parameters
        // instead of storing it in texture in ray tracing pass, to save bandwidth.
        float3 rayDirection = ProbeSamplingVector(rayIdx, PassDataCB.ProbeField);
        float4 rayHitInfo = RayHitInfo[rayIdx];
        float3 rayHitRadiance = rayHitInfo.rgb * EnergyConservation;

        //float3 rayHitLocation = rayHitInfo.w * rayDirection + probePosition;

        // Why do I need this????
        //Vector3 rayHitNormal = sampleTextureFetch(rayHitNormals, C, 0).xyz;
        //rayHitLocation += rayHitNormal * 0.01f;

        // Detect misses and force depth;
        float rayProbeDistance = rayHitInfo.w < 0.0 ? FloatMax : rayHitInfo.w;

        // Since irradiance probe is smaller than depth one, not all SIMD warps in the group will hit this branch, 
        // but all thread in a warp will always agree as long as irradiance probe size is a multiple of the depth one
        if (all(probeLocal2DTexelIndex < IrradianceProbeSize))
        {
            // Using depth probe texel index 
            float3 irradianceTexelDirection = OctDecode(NormalizedIrradianceProbeOctCoord(probeLocal2DTexelIndex, PassDataCB.ProbeField));
            float irradianceWeight = max(0.0, dot(irradianceTexelDirection, rayDirection));

            if (irradianceWeight > Epsilon)
                irradianceResult += float4(rayHitRadiance * irradianceWeight, irradianceWeight);
        }

        float3 depthTexelDirection = OctDecode(NormalizedDepthProbeOctCoord(probeLocal2DTexelIndex, PassDataCB.ProbeField));
        float depthWeight = pow(max(0.0, dot(depthTexelDirection, rayDirection)), DepthSharpness);

        depthResult += float4(rayProbeDistance * depthWeight, Square(rayProbeDistance) * depthWeight, 0.0, depthWeight);
    }

    if (all(probeLocal2DTexelIndex < IrradianceProbeSize))
    {
        //if (irradianceResult.w > Epsilon)
        {
            irradianceResult.rgb *= FourPi / RaysPerProbe; // PDF is 1 / (4 * Pi)

            RWTexture2D<float4> atlas = RW_Float4_Textures2D[PassDataCB.ProbeField.IrradianceProbeAtlasTexIdx];

            float hysteresis = 0.0; // Replace by configurable parameter
            float alpha = 1.0 - hysteresis;

            uint2 texelIndex = IrradianceProbeAtlasTexelIndex(probeIndex, probeLocal2DTexelIndex, PassDataCB.ProbeField);
            float3 previousIrradiance = atlas[texelIndex].rgb;
            float3 newIrradiance = lerp(previousIrradiance, irradianceResult.rgb, alpha);

            atlas[texelIndex].rgb = newIrradiance;
        }
    }

    //if (depthResult.w > Epsilon)
    {
        depthResult.xy /= depthResult.w;
       
        RWTexture2D<float4> atlas = RW_Float4_Textures2D[PassDataCB.ProbeField.DepthProbeAtlasTexIdx];

        float hysteresis = 0.0; // Replace by configurable parameter
        float alpha = 1.0 - hysteresis;
        uint2 texelIndex = DepthProbeAtlasTexelIndex(probeIndex, probeLocal2DTexelIndex, PassDataCB.ProbeField);
        float2 previousDepth = atlas[texelIndex].rg;
        float2 newDepth = lerp(previousDepth, depthResult.xy, alpha);

        atlas[texelIndex].rg = newDepth;
    }
}

#endif