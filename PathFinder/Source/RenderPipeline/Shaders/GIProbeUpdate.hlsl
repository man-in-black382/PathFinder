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
static const uint RaysPerProbe = 144;
static const float EnergyConservation = 0.95;
static const float DepthSharpness = 1.0;
static const float SignificantChangeThreshold = 0.25;
static const float NewDistributionChangeThreshold = 0.8;

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

    uint adjustedProbeIndex = UInt4_Textures2D[PassDataCB.ProbeField.IndirectionTableTexIdx][uint2(probeIndex, 0)].r;

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
        float3 rayHitRadiance = rayHitInfo.rgb;// *EnergyConservation;

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
        if (irradianceResult.w > Epsilon)
        {
            irradianceResult.rgb *= FourPi / RaysPerProbe; // PDF is 1 / (4 * Pi)
            irradianceResult.rgb = EncodeProbeIrradiance(irradianceResult.rgb);

            RWTexture2D<float4> atlas = RW_Float4_Textures2D[PassDataCB.ProbeField.IrradianceProbeAtlasTexIdx];
            uint2 texelIndex = IrradianceProbeAtlasTexelIndex(adjustedProbeIndex, probeLocal2DTexelIndex, PassDataCB.ProbeField);

            float3 previousIrradiance = atlas[texelIndex].rgb;
            float hysteresis = 0.98;

            /*float2 lums = float2(CIELuminance(previousIrradiance.rgb), CIELuminance(irradianceResult.rgb));
            float changeMagnitude = abs(lums.x - lums.y) / Max(lums);*/
            float changeMagnitude = Max(abs(previousIrradiance.rgb - irradianceResult.rgb));

            // Lower the hysteresis when a large change is detected
            if (changeMagnitude > SignificantChangeThreshold)
                hysteresis = max(0, hysteresis - 0.15);

            if (changeMagnitude > NewDistributionChangeThreshold)
                hysteresis = 0.0f;

            atlas[texelIndex].rgb = lerp(irradianceResult.rgb, previousIrradiance, hysteresis);
        }
    }

    if (depthResult.w > Epsilon)
    {
        depthResult.xy /= depthResult.w;
       
        RWTexture2D<float4> atlas = RW_Float4_Textures2D[PassDataCB.ProbeField.DepthProbeAtlasTexIdx];

        float hysteresis = 0.98;
        uint2 texelIndex = DepthProbeAtlasTexelIndex(adjustedProbeIndex, probeLocal2DTexelIndex, PassDataCB.ProbeField);
        float2 previousDepth = atlas[texelIndex].rg;

        atlas[texelIndex].rg = lerp(depthResult.xy, previousDepth, hysteresis);
    }

    SetDataInspectorWriteCondition(probeIndex == 5);
    OutputDataInspectorValue(float3(3.456, 1.0, 555.7685)); 
    OutputDataInspectorValue(float2(3.456, 874.21));
    OutputDataInspectorValue(0.15874);
}

#endif