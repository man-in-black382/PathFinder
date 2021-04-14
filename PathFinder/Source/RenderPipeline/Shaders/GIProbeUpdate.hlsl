#ifndef _GIProbeUpdate__
#define _GIProbeUpdate__

#include "GIProbeHelpers.hlsl"
#include "Packing.hlsl"
#include "ColorConversion.hlsl"

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
static const uint RaysPerProbe = 144; // *Must* match values in ProbeField struct
static const float DepthSharpness = 5.0;
static const float SignificantChangeThreshold = 1.0;
static const float NewDistributionChangeThreshold = 2.0;

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

    int3 previous3DIndex = probe3DIndex + PassDataCB.ProbeField.SpawnedProbePlanesCount;
    uint previousProbeIndex = Probe1DIndexFrom3D(previous3DIndex, PassDataCB.ProbeField);
    bool isNewlySpawnedProbe = any(previous3DIndex < 0) || any(previous3DIndex >= PassDataCB.ProbeField.GridSize);

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

    float3 irradianceTexelDirection = OctDecode(NormalizedIrradianceProbeOctCoord(probeLocal2DTexelIndex, PassDataCB.ProbeField));
    float3 depthTexelDirection = OctDecode(NormalizedDepthProbeOctCoord(probeLocal2DTexelIndex, PassDataCB.ProbeField));

    // Switch to disable probes inside of geometry
    float backfaceWeight = 1.0;
    float maxRayLength = length(PassDataCB.ProbeField.CellSize.xxx);

    // For each ray
    for (uint rayIdx = 0; rayIdx < RaysPerProbe; ++rayIdx)
    {
        // We restore ray direction here knowing current frame ray generation parameters
        // instead of storing it in texture in ray tracing pass, to save bandwidth.
        float3 rayDirection = ProbeSamplingVector(rayIdx, PassDataCB.ProbeField);
        float4 rayHitInfo = RayHitInfo[rayIdx];
        float3 rayHitRadiance = max(rayHitInfo.rgb, 0.0001)/* * EnergyConservation*/;

        // Detect misses and force depth
        float rayProbeDistance = rayHitInfo.w == ProbeRayBackfaceIndicator ?
            maxRayLength : min(rayHitInfo.w, maxRayLength);

        // If at least one of the rays has hit a geometry backface,
        // we consider this probe problematic and disable it completely via backface weight.
        if (rayHitInfo.w == ProbeRayBackfaceIndicator)
            backfaceWeight = 0.0;

        // Since irradiance probe is smaller than depth one, not all SIMD warps in the group will hit this branch, 
        // but all thread in a warp will always agree as long as irradiance probe size is a multiple of the depth one
        if (all(probeLocal2DTexelIndex < IrradianceProbeSize))
        {
            // Using depth probe texel index 
            float irradianceWeight = max(0.0, dot(irradianceTexelDirection, rayDirection));

            if (irradianceWeight > Epsilon)
                irradianceResult += float4(rayHitRadiance * irradianceWeight, irradianceWeight);
        }

        float depthWeight = pow(max(0.0, dot(depthTexelDirection, rayDirection)), DepthSharpness);
        depthResult += float4(rayProbeDistance * depthWeight, Square(rayProbeDistance) * depthWeight, 0.0, depthWeight);
    }

    if (all(probeLocal2DTexelIndex < IrradianceProbeSize))
    {
        if (irradianceResult.w > Epsilon)
        {
            irradianceResult.rgb *= FourPi / RaysPerProbe; // PDF is 1 / (4 * Pi)
            irradianceResult.rgb = EncodeProbeIrradiance(irradianceResult.rgb);

            Texture2D previousAtlas = Textures2D[PassDataCB.ProbeField.PreviousIrradianceProbeAtlasTexIdx];
            RWTexture2D<float4> currentAtlas = RW_Float4_Textures2D[PassDataCB.ProbeField.CurrentIrradianceProbeAtlasTexIdx];

            uint2 texelIndex = IrradianceProbeAtlasTexelIndex(probeIndex, probeLocal2DTexelIndex, PassDataCB.ProbeField);
            uint2 previousTexelIndex = texelIndex;

            float hysteresis = 0.98;

            if (isNewlySpawnedProbe)
            {
                hysteresis = 0.0;
            }
            else
            {
                previousTexelIndex = IrradianceProbeAtlasTexelIndex(previousProbeIndex, probeLocal2DTexelIndex, PassDataCB.ProbeField);
            }

            float4 previousIrradianceAndBackfaceWeight = previousAtlas[previousTexelIndex];
            float3 previousIrradiance = previousIrradianceAndBackfaceWeight.rgb;
            float previousBackfaceWeight = previousIrradianceAndBackfaceWeight.w;

            float changeMagnitude = Max(abs(previousIrradiance.rgb - irradianceResult.rgb));

            // Lower the hysteresis when a large change is detected
          /*  if (changeMagnitude > SignificantChangeThreshold)
                hysteresis = max(0, hysteresis - 0.05);

            if (changeMagnitude > NewDistributionChangeThreshold)
                hysteresis = 0.0f;*/

            if (isNewlySpawnedProbe)
            {
                hysteresis = 0.0;
            }

            float newBackfaceWeight = lerp(backfaceWeight, previousBackfaceWeight, hysteresis);
            float3 newIrradiance = lerp(irradianceResult.rgb, previousIrradiance, hysteresis);

            currentAtlas[texelIndex] = float4(newIrradiance, newBackfaceWeight);
        }
    }

    if (depthResult.w > Epsilon)
    {
        depthResult.xy /= depthResult.w;

        Texture2D previousAtlas = Textures2D[PassDataCB.ProbeField.PreviousDepthProbeAtlasTexIdx];
        RWTexture2D<float4> currentAtlas = RW_Float4_Textures2D[PassDataCB.ProbeField.CurrentDepthProbeAtlasTexIdx];

        uint2 texelIndex = DepthProbeAtlasTexelIndex(probeIndex, probeLocal2DTexelIndex, PassDataCB.ProbeField);
        uint2 previousTexelIndex = texelIndex;
        float hysteresis = 0.98;

        if (isNewlySpawnedProbe) 
        {
            hysteresis = 0.0;
        }
        else
        {
            previousTexelIndex = DepthProbeAtlasTexelIndex(previousProbeIndex, probeLocal2DTexelIndex, PassDataCB.ProbeField);
        }
        
        float2 previousDepth = previousAtlas[previousTexelIndex].rg;

        currentAtlas[texelIndex].rg = lerp(depthResult.xy, previousDepth, hysteresis);
    }
}

#endif